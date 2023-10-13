#include "Assembler.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "Buffer.h"

#include <cstdlib>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

static ProcessorErrorCode CompileLine (Buffer *binaryBuffer, Buffer *listingBuffer, TextLine *line);
static ProcessorErrorCode GetInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments, int *offset);
static ProcessorErrorCode GetInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments);
static ProcessorErrorCode EmitInstruction (Buffer *binaryBuffer, Buffer *listingBiffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine);

static ProcessorErrorCode WriteDataToFiles (Buffer *binaryBuffer, Buffer *listingBuffer, int binaryDescriptor, int listingDescriptor);
static ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor);

static ProcessorErrorCode CreateAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer, FileBuffer *sourceFile, TextBuffer *sourceText);
static ProcessorErrorCode DestroyAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer);

static ssize_t CountWhitespaces (TextLine *line);
static ssize_t FindActualStringEnd (TextLine *line);
static ssize_t FindActualStringBegin (TextLine *line);

// TODO add compilation header

ProcessorErrorCode AssembleFile (TextBuffer *text, FileBuffer *file, int binaryDescriptor, int listingDescriptor) {
    PushLog (1);

    custom_assert (file,        pointer_is_null, NO_BUFFER);
    custom_assert (text,        pointer_is_null, NO_BUFFER);
    custom_assert (text->lines, pointer_is_null, NO_BUFFER);

    Buffer binaryBuffer  {0, 0, NULL};
    Buffer listingBuffer {0, 0, NULL};

    ON_DEBUG (PrintSuccessMessage ("Starting assembly...", NULL));

    ErrorFound (CreateAssemblyBuffers (&binaryBuffer, &listingBuffer, file, text), "Error occuried while creating file buffers");

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    for (size_t lineIndex = 0; lineIndex < text->line_count; lineIndex++) {
        errorCode = CompileLine (&binaryBuffer, &listingBuffer, text->lines + lineIndex);

        if (!(errorCode & (BLANK_LINE)) && errorCode != NO_PROCESSOR_ERRORS) {
            errorCode = (ProcessorErrorCode) (errorCode | DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer));
            ErrorFound (errorCode, "Compilation error");
        }
    }

    errorCode = WriteDataToFiles (&binaryBuffer, &listingBuffer, binaryDescriptor, listingDescriptor);

    if (errorCode != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (errorCode | DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer));
        ErrorFound (errorCode, "Error occuried while writing data to output file");
    }

    ErrorFound (DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer), "Error occuried while destroying assembly buffers");

    PrintSuccessMessage ("Assembly finished successfully!", NULL);
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileLine (Buffer *binaryBuffer, Buffer *listingBuffer, TextLine *line) {
    PushLog (2);
    custom_assert (line,          pointer_is_null, NO_BUFFER);
    custom_assert (line->pointer, pointer_is_null, NO_BUFFER);

    if (FindActualStringBegin (line) < 0) {
        RETURN BLANK_LINE;
    }

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    InstructionArguments arguments {NAN, REGISTER_COUNT};
    ArgumentsType permittedArguments = NO_ARGUMENTS;
    AssemblerInstruction outputInstruction {"", {0, 0}, NULL};
    int argumentsOffset = 0;

    if ((errorCode = GetInstructionOpcode (line, &outputInstruction, &permittedArguments, &argumentsOffset)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    #ifndef _NDEBUG
        char message [128] = "";
        sprintf (message, "Instruction found: %s", outputInstruction.instructionName);
        PrintInfoMessage (message, NULL);
    #endif

    if ((errorCode = GetInstructionArgumentsData (&outputInstruction, line, &arguments, permittedArguments)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    RETURN EmitInstruction (binaryBuffer, listingBuffer, &outputInstruction, &arguments, line);
}

static ProcessorErrorCode CreateAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer, FileBuffer *sourceFile, TextBuffer *sourceText) {
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
    const size_t MaxBinaryAllocationCoefficient =  3;

    binaryBuffer->capacity = (size_t) sourceFile->buffer_size * MaxBinaryAllocationCoefficient;
    binaryBuffer->data = (char *) calloc (binaryBuffer->capacity, sizeof (char));

    if (!binaryBuffer->data) {
        ErrorFound (NO_BUFFER, "Unable to create binary file buffer");
    }

    //Listing format:
    // ip   opcode  line
    //0000    00    push ...
    //listing line size = source line size + 14 bytes data + 1 '\0' byte = source line size + 15 bytes

    size_t listingAllocationSize = 0;

    for (size_t lineIndex = 0; lineIndex < sourceText->line_count; lineIndex++) {
        listingAllocationSize += sourceText->lines [lineIndex].length + 20;
    }

    listingBuffer->capacity = listingAllocationSize + HEADER_SIZE;
    listingBuffer->data = (char *) calloc (listingBuffer->capacity, sizeof (char));

    if (!listingBuffer->data) {
        free (binaryBuffer->data);
        ErrorFound (NO_BUFFER, "Unable to create listing file buffer");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer) {
    PushLog (4);

    if (binaryBuffer) {
        free (binaryBuffer->data);
    }

    if (listingBuffer) {
        free (listingBuffer->data);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDataToFiles (Buffer *binaryBuffer, Buffer *listingBuffer, int binaryDescriptor, int listingDescriptor) {
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

    // Three times header_size should be enough for header and header field's names
    // I think so...
    // TODO try to calculate needed buffer size
    char *header = (char *) calloc(HEADER_SIZE * 3 + 3, sizeof (char));

    if (!header) {
        ErrorFound (NO_BUFFER, "Can not allocate memory for temporary header buffer");
    }

    #define HEADER_FIELD(FIELD_NAME, FIELD_VALUE, ...) \
            strcat (header, #FIELD_VALUE);

    #include "AssemblerHeader.def"

    #undef HEADER_FIELD

    if (!WriteBuffer (binaryDescriptor, header, (ssize_t) HEADER_SIZE)) {
        free (header);
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to binary file");
    }

    header [0] = '\0';

    #define HEADER_FIELD(FIELD_NAME, FIELD_VALUE, ...) \
            strcat (header, #FIELD_NAME ": " #FIELD_VALUE "\n");

        #include "AssemblerHeader.def"

    #undef HEADER_FIELD

    strcat (header, "\n\n");


    if (listingDescriptor != -1) {
        const char HeaderLegend [] = "HEADER:\n";

        if (!WriteBuffer (listingDescriptor, HeaderLegend, (ssize_t) sizeof (HeaderLegend) - 1)) {
            free (header);
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
        }

        if (!WriteBuffer (listingDescriptor, header, (ssize_t) strlen (header))) {
            free (header);
            ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
        }
    }

    free (header);
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode GetInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments, int *offset) {
    PushLog (3);

    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (line,        pointer_is_null, NO_BUFFER);

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    int sscanfResult = sscanf (line->pointer, "%s%n", sscanfBuffer, offset);

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

static ProcessorErrorCode GetInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments) {
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

                #include "Registers.def"

                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
            }else {
                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");
            }
            break;

        case 0:
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%*s %lf", &arguments->immedArgument) > 0){
                if (!(permittedArguments & IMMED_ARGUMENT)) {
                    ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
                }
                instruction->commandCode.arguments = IMMED_ARGUMENT;

            } else if (sscanf (line->pointer + offset, "%*s %s", registerNameBuffer) > 0) {
                if (!(permittedArguments & REGISTER_ARGUMENT)) {
                    ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
                }

                instruction->commandCode.arguments = REGISTER_ARGUMENT;

                #include "Registers.def"

                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
            }else {
                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");
            }
            break;

        default:
            ErrorFound (TOO_MANY_ARGUMENTS, "Too manu arguments for this command");
            break;
    }

    #undef REGISTER

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitInstruction (Buffer *binaryBuffer, Buffer *listingBiffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine) {
    PushLog (3);
    custom_assert (instruction,        pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (binaryBuffer,       pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (binaryBuffer->data, pointer_is_null, WRONG_INSTRUCTION);

    const size_t ServiceInfoLength = 30;
    char listingInfoBuffer [MAX_INSTRUCTION_LENGTH + ServiceInfoLength] = "";

    sprintf (listingInfoBuffer, "%.4lu\t%.2x\t\t%s\n", binaryBuffer->currentIndex, *(unsigned char *) &instruction->commandCode, sourceLine->pointer + FindActualStringBegin (sourceLine));
    WriteDataToBuffer (listingBiffer, listingInfoBuffer, strlen (listingInfoBuffer));

    WriteDataToBuffer (binaryBuffer, &instruction->commandCode, sizeof (CommandCode));

    ON_DEBUG (char message [128] = "Arguments: ");

    if (instruction->commandCode.arguments & REGISTER_ARGUMENT) {
        WriteDataToBuffer (binaryBuffer, &arguments->registerIndex, sizeof (char));

        char *registerName = NULL;

        #define REGISTER(NAME, INDEX)                       \
                if (arguments->registerIndex == INDEX) {    \
                    registerName = #NAME;                   \
                }

        #include "Registers.def"

        #undef REGISTER

        sprintf (message + strlen (message), "%s (size = %lu) ", registerName, sizeof (char));
    }

    if (instruction->commandCode.arguments & IMMED_ARGUMENT) {
        WriteDataToBuffer (binaryBuffer, &arguments->immedArgument, sizeof (elem_t));

        sprintf (message + strlen (message), "%lf (size = %lu)", arguments->immedArgument, sizeof (elem_t));
    }

    // check if message != "Arguments: "
    ON_DEBUG (if (message [12] != '\0'){ PrintInfoMessage (message, NULL);})

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)             \
                if (instruction->commandCode.opcode == ((CommandCode) COMMAND_CODE).opcode)     \
                    ASSEMBLER_CALLBACK                                                          \


    #include "Instructions.def"

    #undef INSTRUCTION
    #undef WriteDataToBuffer

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
