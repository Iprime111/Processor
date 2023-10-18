#include "Assembler.h"
#include "AssemblyHeader.h"
#include "Buffer.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "Buffer.h"
#include "DSLFunctions.h"
#include <Label.h>

#include <cstdlib>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

const size_t MAX_LABELS_COUNT = 128;

static ProcessorErrorCode CompileLine (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, TextLine *line);
static ProcessorErrorCode GetInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments);
static ProcessorErrorCode GetInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments, Buffer <Label> *labelsBuffer);
static ProcessorErrorCode EmitInstruction (Buffer <char> *binaryBuffer, Buffer <char> *listingBiffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine);

static ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, int binaryDescriptor, int listingDescriptor);
static ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor);

static ProcessorErrorCode CreateAssemblyBuffers  (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, FileBuffer *sourceFile, TextBuffer *sourceText);
static ProcessorErrorCode DestroyAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer);

static ssize_t CountWhitespaces (TextLine *line);
static ssize_t FindActualStringEnd (TextLine *line);
static ssize_t FindActualStringBegin (TextLine *line);

ProcessorErrorCode AssembleFile (TextBuffer *text, FileBuffer *file, int binaryDescriptor, int listingDescriptor) {
    PushLog (1);

    custom_assert (file,        pointer_is_null, NO_BUFFER);
    custom_assert (text,        pointer_is_null, NO_BUFFER);
    custom_assert (text->lines, pointer_is_null, NO_BUFFER);

    Buffer <char>  binaryBuffer  {0, 0, NULL};
    Buffer <char>  listingBuffer {0, 0, NULL};
    Buffer <Label> labelsBuffer  {0, 0, NULL};

    ON_DEBUG (PrintSuccessMessage ("Starting assembly...", NULL));

    ErrorFound (CreateAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer, file, text), "Error occuried while creating file buffers");

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    for (size_t lineIndex = 0; lineIndex < text->line_count; lineIndex++) {
        errorCode = CompileLine (&binaryBuffer, &listingBuffer, &labelsBuffer, text->lines + lineIndex);

        if (!(errorCode & (BLANK_LINE)) && errorCode != NO_PROCESSOR_ERRORS) {
            errorCode = (ProcessorErrorCode) (errorCode | DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer));
            ErrorFound (errorCode, "Compilation error");
        }
    }

    errorCode = WriteDataToFiles (&binaryBuffer, &listingBuffer, binaryDescriptor, listingDescriptor);

    if (errorCode != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (errorCode | DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer));
        ErrorFound (errorCode, "Error occuried while writing data to output file");
    }

    ErrorFound (DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer), "Error occuried while destroying assembly buffers");

    PrintSuccessMessage ("Assembly finished successfully!", NULL);
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileLine (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, TextLine *line) {
    PushLog (2);

    custom_assert (line,          pointer_is_null, NO_BUFFER);
    custom_assert (line->pointer, pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer,  pointer_is_null, NO_BUFFER);

    if (FindActualStringBegin (line) < 0) {
        RETURN BLANK_LINE;
    }

    char labelName [LABEL_NAME_LENGTH] = "";
    int labelNameLength = 0;

    if (line->pointer [FindActualStringEnd (line) - 1] == ':' && sscanf (line->pointer, "%s%n", labelName, &labelNameLength) > 0) {
        Label label {};
        labelName [labelNameLength - 1] = '\0';
        InitLabel (&label, labelName, (long long) binaryBuffer->currentIndex);

        WriteDataToBufferErrorCheck ("Error occuried while writing label to buffer", labelsBuffer, &label, 1);

        RETURN NO_PROCESSOR_ERRORS;
    }

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    InstructionArguments arguments {NAN, REGISTER_COUNT};
    ArgumentsType permittedArguments = NO_ARGUMENTS;
    AssemblerInstruction outputInstruction {"", {0, 0}, NULL};

    if ((errorCode = GetInstructionOpcode (line, &outputInstruction, &permittedArguments)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    #ifndef _NDEBUG
        char message [128] = "";
        sprintf (message, "Instruction found: %s", outputInstruction.instructionName);
        PrintInfoMessage (message, NULL);
    #endif

    if ((errorCode = GetInstructionArgumentsData (&outputInstruction, line, &arguments, permittedArguments, labelsBuffer)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    RETURN EmitInstruction (binaryBuffer, listingBuffer, &outputInstruction, &arguments, line);
}

static ProcessorErrorCode CreateAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, FileBuffer *sourceFile, TextBuffer *sourceText) {
    PushLog (3);

    custom_assert (binaryBuffer,  pointer_is_null, OUTPUT_FILE_ERROR);
    custom_assert (listingBuffer, pointer_is_null, OUTPUT_FILE_ERROR);
    custom_assert (sourceFile,    pointer_is_null, NO_BUFFER);

    // maximum difference between source and binary:
    // 2 sym command + 1 sym argument + 1 whitespace = 4 bytes in source
    // 1 byte command + 8 bytes arguments = 9 bytes in binary
    //                  |
    //                 \ /
    // max allocation coefficient = 9 / 4 = 2.25 ---> coef = 3
    const size_t MaxBinaryAllocationCoefficient = 3;

    ErrorFound (InitBuffer (binaryBuffer, (size_t) sourceFile->buffer_size * MaxBinaryAllocationCoefficient), "Unable to create binary file buffer");

    //Listing format:
    // ip   opcode  line
    //0000    00    push ...
    //listing line size = source line size + 14 bytes data + 1 '\0' byte = source line size + 15 bytes

    size_t listingAllocationSize = 0;

    for (size_t lineIndex = 0; lineIndex < sourceText->line_count; lineIndex++) {
        listingAllocationSize += sourceText->lines [lineIndex].length + 30;
    }

    const size_t MaxHeaderAllocationCoefficient = 2;

    ErrorFound (InitBuffer (listingBuffer, listingAllocationSize + sizeof (Header) * MaxHeaderAllocationCoefficient), "Unable to create listing file buffer");

    // Allocating labels buffer

    ErrorFound (InitBuffer (labelsBuffer, MAX_LABELS_COUNT), "Unable to create labels buffer");

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer) {
    PushLog (4);

    custom_assert (binaryBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer,  pointer_is_null, NO_BUFFER);

    DestroyBuffer (binaryBuffer);
    DestroyBuffer (listingBuffer);
    DestroyBuffer (labelsBuffer);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, int binaryDescriptor, int listingDescriptor) {
    PushLog (2);

    custom_assert (binaryBuffer,           pointer_is_null,   NO_BUFFER);
    custom_assert (listingBuffer,          pointer_is_null,   NO_BUFFER);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    ErrorFound (WriteHeader (binaryDescriptor, listingDescriptor), "Error occuried while writing header");

    if (!WriteBuffer (binaryDescriptor, binaryBuffer->data, (ssize_t) binaryBuffer->currentIndex)) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to binary file");
    }

    if (listingDescriptor != -1) {
        const char *ListingLegend = " ip \topcode\tsource\n";

        if (!WriteBuffer (listingDescriptor, ListingLegend, (ssize_t) strlen (ListingLegend))) {
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to listing file");
        }

        if (!WriteBuffer (listingDescriptor, listingBuffer->data, (ssize_t) listingBuffer->currentIndex)) {
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to listing file");
        }
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor) {
    PushLog (2);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    Header header {};
    InitHeader (&header);

    if (!WriteBuffer (binaryDescriptor, (char *) &header, sizeof (header))) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to binary file");
    }

    if (listingDescriptor != -1) {
        const char HeaderLegend [] = "HEADER:\n";

        if (!WriteBuffer (listingDescriptor, HeaderLegend, (ssize_t) sizeof (HeaderLegend) - 1)) {
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
        }

        Buffer <char> listingHeaderBuffer {};
        InitBuffer (&listingHeaderBuffer, sizeof (header) * 4); // TODO Hope 4x sizeof will be enough

        WriteHeaderField (&listingHeaderBuffer, &header, signature, sizeof (unsigned short));
        WriteHeaderField (&listingHeaderBuffer, &header, version,   sizeof (VERSION) - 1);
        WriteHeaderField (&listingHeaderBuffer, &header, byteOrder, sizeof (SYSTEM_BYTE_ORDER) - 1);

        WriteDataToBufferErrorCheck ("Error occuried while writing new line to listing file", &listingHeaderBuffer, "\n\n", 2);

        if (!WriteBuffer (listingDescriptor, listingHeaderBuffer.data, (ssize_t) listingHeaderBuffer.currentIndex)) {
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
        }

        DestroyBuffer (&listingHeaderBuffer);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode GetInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments) {
    PushLog (3);

    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (line,        pointer_is_null, NO_BUFFER);

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    int sscanfResult = sscanf (line->pointer, "%s", sscanfBuffer);

    if (sscanfResult <= 0) {
        RETURN BLANK_LINE;
    }

    if (sscanfResult == EOF) {
        RETURN NO_BUFFER;
    }

    const AssemblerInstruction *constInstruction = FindInstructionByName (sscanfBuffer);

    if (!constInstruction) {
        ErrorFound (WRONG_INSTRUCTION, "Wrong instruction has been read");
    }

    instruction->instructionName = constInstruction->instructionName;
    instruction->commandCode.opcode = constInstruction->commandCode.opcode;
    instruction->callbackFunction = constInstruction->callbackFunction;

    *permittedArguments = (ArgumentsType) constInstruction->commandCode.arguments;

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode GetInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments, Buffer <Label> *labelsBuffer) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ssize_t argumentsCount = CountWhitespaces (line);

    if (argumentsCount < 0) {
        RETURN TOO_FEW_ARGUMENTS;
    }

    *arguments = {NAN, REGISTER_COUNT};

    TextLine argsLine {line->pointer, line->length };
    ssize_t offset = FindActualStringBegin (&argsLine);

    char registerNameBuffer [MAX_INSTRUCTION_LENGTH] = "";

    bool isArgumentReadCorrectly = false;

    #define REGISTER(NAME, INDEX)                           \
                if (!strcmp (#NAME, registerNameBuffer)) {  \
                    arguments->registerIndex = INDEX;       \
                    break;                                  \
                }

    switch (argumentsCount) {
        case 2:
            if (permittedArguments != (REGISTER_ARGUMENT | IMMED_ARGUMENT)) {
                ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
            }

            if (sscanf (line->pointer + offset, "%*s %3s+%lf", registerNameBuffer, &arguments->immedArgument) > 0) {
                instruction->commandCode.arguments = IMMED_ARGUMENT | REGISTER_ARGUMENT;
                isArgumentReadCorrectly = true;

                #include "Registers.def"

                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
            }else {
                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");
            }
            break;

        case 0:
            isArgumentReadCorrectly = true;
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%*s %lf", &arguments->immedArgument) > 0){
                if (!(permittedArguments & IMMED_ARGUMENT)) {
                    ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
                }
                instruction->commandCode.arguments = IMMED_ARGUMENT;

            } else if (sscanf (line->pointer + offset, "%*s %s", registerNameBuffer) > 0) {
                if (!(permittedArguments & REGISTER_ARGUMENT)) {
                    break;
                }

                isArgumentReadCorrectly = true;
                instruction->commandCode.arguments = REGISTER_ARGUMENT;

                #include "Registers.def"

                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
            }else {
                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");
            }

            isArgumentReadCorrectly = true;
            break;

        default:
            ErrorFound (TOO_MANY_ARGUMENTS, "Too many arguments for this command");
            break;
    }

    #undef REGISTER

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, ...)        \
                if (instruction->commandCode.opcode == ((CommandCode) COMMAND_CODE).opcode)     \
                    ASSEMBLER_CALLBACK                                                          \


    #include "Instructions.def"

    #undef INSTRUCTION

    if (!isArgumentReadCorrectly) {
        ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitInstruction (Buffer <char> *binaryBuffer, Buffer <char> *listingBiffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine) {
    PushLog (3);
    custom_assert (instruction,        pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (binaryBuffer,       pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (binaryBuffer->data, pointer_is_null, WRONG_INSTRUCTION);

    const size_t ServiceInfoLength = 30;
    char listingInfoBuffer [MAX_INSTRUCTION_LENGTH + ServiceInfoLength] = "";

    sprintf (listingInfoBuffer, "%.4lu\t%.2x\t\t%s\n", binaryBuffer->currentIndex, *(unsigned char *) &instruction->commandCode, sourceLine->pointer + FindActualStringBegin (sourceLine));
    WriteDataToBufferErrorCheck ("Error occuried while writing instruction to listing buffer", listingBiffer, listingInfoBuffer, strlen (listingInfoBuffer));

    WriteDataToBufferErrorCheck ("Error occuried while writing instruction to binary buffer", binaryBuffer, &instruction->commandCode, sizeof (CommandCode));

    ON_DEBUG (char message [128] = "Arguments: ");

    if (instruction->commandCode.arguments & REGISTER_ARGUMENT) {
        WriteDataToBufferErrorCheck ("Error occuried while writing register number to binary buffer", binaryBuffer, &arguments->registerIndex, sizeof (char));

        char *registerName = NULL;

        #define REGISTER(NAME, INDEX)                       \
                if (arguments->registerIndex == INDEX) {    \
                    registerName = #NAME;                   \
                }

        #include "Registers.def"

        #undef REGISTER

        ON_DEBUG (sprintf (message + strlen (message), "%s (size = %lu) ", registerName, sizeof (char)));
    }

    if (instruction->commandCode.arguments & IMMED_ARGUMENT) {
        WriteDataToBufferErrorCheck ("Error occuried while writing immed argument to binary buffer", binaryBuffer, &arguments->immedArgument, sizeof (elem_t));

        ON_DEBUG (sprintf (message + strlen (message), "%lf (size = %lu)", arguments->immedArgument, sizeof (elem_t)));
    }

    // checking if message != "Arguments: "
    ON_DEBUG (if (message [12] != '\0'){ PrintInfoMessage (message, NULL);})

    RETURN NO_PROCESSOR_ERRORS;
}

static ssize_t CountWhitespaces (TextLine *line) {
    PushLog (3);

    custom_assert (line,          pointer_is_null, -1);
    custom_assert (line->pointer, pointer_is_null, -1);

    ssize_t lineBegin = FindActualStringBegin (line);
    ssize_t lineEnd   = FindActualStringEnd   (line);

    if (lineBegin < 0 || lineEnd < 0) {
        RETURN -1;
    }

    ssize_t spaceCount = 0;

    for (ssize_t charIndex = lineBegin; charIndex < lineEnd; charIndex++) {
        if (isspace (line->pointer [charIndex]) || line->pointer [charIndex] == '+')
            spaceCount++;
    }

    RETURN spaceCount;
}

#define DetectWhitespacePosition(FOR_PREDICATE)                 \
            custom_assert (line,          pointer_is_null, -1); \
            custom_assert (line->pointer, pointer_is_null, -1); \
            for (FOR_PREDICATE) {                               \
                if (!isspace (line->pointer [charPointer])){    \
                    RETURN (ssize_t) charPointer;               \
                }                                               \
            }                                                   \
            RETURN -1

static ssize_t FindActualStringEnd (TextLine *line) {
    PushLog (3);

    size_t length = line->length;

    char *splitterPointer = strchr (line->pointer, ';');
    if (splitterPointer) {
        length = (size_t) (splitterPointer - line->pointer - 1);
    }

    DetectWhitespacePosition (ssize_t charPointer = (ssize_t) length; charPointer >= 0; charPointer--);
}

static ssize_t FindActualStringBegin (TextLine *line) {
    PushLog (3);

    DetectWhitespacePosition (size_t charPointer = 0; charPointer < line->length; charPointer++);
}

#undef DetectWhitespacePosition

// Instruction callback functions

#define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)                       \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                return NO_PROCESSOR_ERRORS;                                                     \
            }

#include "Instructions.def"

#undef INSTRUCTION
