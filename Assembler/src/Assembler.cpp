#include "Assembler.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "Buffer.h"

#include <cstdlib>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

static ProcessorErrorCode CompileLine (Buffer *binaryBuffer, Buffer *listingBuffer, TextLine *line);
static ProcessorErrorCode GetInstructionOpcode (TextLine *line, const AssemblerInstruction **instruction, int *offset);
static ProcessorErrorCode GetInstructionArgumentsData (const AssemblerInstruction *instruction, TextLine *line, CommandCode *commandCode, char *registerIndex, elem_t *immedArgument);
static ProcessorErrorCode EmitInstruction (Buffer *binaryBuffer, Buffer *listingBiffer, CommandCode *commandCode, char registerIndex, elem_t immedArgument);

static ProcessorErrorCode WriteDataToFiles (Buffer *binaryBuffer, Buffer *listingBuffer, int binaryDescriptor, int listingDescriptor);

static ProcessorErrorCode CreateAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer, FileBuffer *sourceFile, TextBuffer *sourceText);
static ProcessorErrorCode DestroyAssemblyBuffers (Buffer *binaryBuffer, Buffer *listingBuffer);

static ssize_t CountWhitespaces (TextLine *line);
static ssize_t FindActualStringEnd (TextLine *line);
static ssize_t FindActualStringBegin (TextLine *line);

ProcessorErrorCode AssembleFile (TextBuffer *text, FileBuffer *file, int binaryDescriptor) {
    PushLog (1);

    custom_assert (file,        pointer_is_null, NO_BUFFER);
    custom_assert (text,        pointer_is_null, NO_BUFFER);
    custom_assert (text->lines, pointer_is_null, NO_BUFFER);

    Buffer binaryBuffer  {0, 0, NULL};
    Buffer listingBuffer {0, 0, NULL};
    CreateAssemblyBuffers (&binaryBuffer, &listingBuffer, file, text);

    for (size_t lineIndex = 0; lineIndex < text->line_count; lineIndex++) {
        ProcessorErrorCode errorCode = CompileLine (&binaryBuffer, &listingBuffer, text->lines + lineIndex);

        if (!(errorCode & (BLANK_LINE))) {
            ErrorFound (errorCode, "Compilation error");
        }
    }

    if (!WriteBuffer (binaryDescriptor, binaryBuffer.data, (ssize_t) binaryBuffer.currentIndex)) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to binary file");
    }

    WriteDataToFiles (&binaryBuffer, &listingBuffer, binaryDescriptor, -1);

    RETURN DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer);
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
        listingAllocationSize += sourceText->lines [lineIndex].length + 15;
    }

    listingBuffer->capacity = listingAllocationSize;
    listingBuffer->data = (char *) calloc (listingBuffer->capacity, sizeof (char));

    if (!listingBuffer->data) {
        free (binaryBuffer);
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

static ProcessorErrorCode CompileLine (Buffer *binaryBuffer, Buffer *listingBuffer, TextLine *line) {
    PushLog (2);
    custom_assert (line,          pointer_is_null, NO_BUFFER);
    custom_assert (line->pointer, pointer_is_null, NO_BUFFER);

    if (FindActualStringBegin (line) < 0) {
        RETURN BLANK_LINE;
    }

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    // Get instruction basic data
    const AssemblerInstruction *instruction = NULL;
    int argumentsOffset = 0;

    if ((errorCode = GetInstructionOpcode (line, &instruction, &argumentsOffset)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    ON_DEBUG (printf ("Instruction found: %s. Arguments: ", instruction->instructionName));

    char registerIndex = -1;
    elem_t immedArgument    = {};
    CommandCode commandCode = {0, 0};

    if ((errorCode = GetInstructionArgumentsData (instruction, line, &commandCode, &registerIndex, &immedArgument)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    RETURN EmitInstruction (binaryBuffer, listingBuffer, &commandCode, registerIndex, immedArgument);
}

static ProcessorErrorCode GetInstructionOpcode (TextLine *line, const AssemblerInstruction **instruction, int *offset) {
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

    *instruction = FindInstructionByName (sscanfBuffer);

    if (!*instruction) {
        ErrorFound (WRONG_INSTRUCTION, "Wrong instruction has been read");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode GetInstructionArgumentsData (const AssemblerInstruction *instruction, TextLine *line, CommandCode *commandCode, char *registerIndex, elem_t *immedArgument) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ssize_t argumentsCount = CountWhitespaces (line);

    if (argumentsCount < 0) {
        RETURN TOO_FEW_ARGUMENTS;
    }

    commandCode->opcode = instruction->commandCode.opcode;

    *registerIndex = -1;
    *immedArgument =  0;

    TextLine argsLine {line->pointer, line->length };
    ssize_t offset = FindActualStringBegin (&argsLine);

    char registerNameBuffer [MAX_INSTRUCTION_LENGTH] = "";

    #define REGISTER(NAME, INDEX)                           \
                if (!strcmp (#NAME, registerNameBuffer)) {  \
                    *registerIndex = INDEX;                 \
                    break;                                  \
                }

    switch (argumentsCount) {
        case 2:
            if (instruction->commandCode.arguments != (REGISTER_ARGUMENT | IMMED_ARGUMENT)) {
                ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
            }

            if (sscanf (line->pointer + offset, "%*s %3s+%lf", registerNameBuffer, immedArgument) > 0) {
                commandCode->arguments = IMMED_ARGUMENT | REGISTER_ARGUMENT;

                #include "Registers.def"

                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
            }else {
                ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");
            }
            break;

        case 0:
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%*s %lf", immedArgument) > 0){
                if (!(instruction->commandCode.arguments & IMMED_ARGUMENT)) {
                    ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
                }
                commandCode->arguments = IMMED_ARGUMENT;

            } else if (sscanf (line->pointer + offset, "%*s %s", registerNameBuffer) > 0) {
                if (!(instruction->commandCode.arguments & REGISTER_ARGUMENT)) {
                    ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
                }

                commandCode->arguments = REGISTER_ARGUMENT;

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

static ProcessorErrorCode EmitInstruction (Buffer *binaryBuffer, Buffer *listingBiffer, CommandCode *commandCode, char registerIndex, elem_t immedArgument) {
    PushLog (3);
    custom_assert (commandCode, pointer_is_null, WRONG_INSTRUCTION);

    const size_t ServiceInfoLength = 30;
    char listingInfoBuffer [MAX_INSTRUCTION_LENGTH + ServiceInfoLength] = "";

    sprintf (listingInfoBuffer, "%.4lu\t%.2x\t%4s\n", binaryBuffer->currentIndex, *(unsigned char *) commandCode, "test");
    WriteDataToBuffer (listingBiffer, listingInfoBuffer, strlen (listingInfoBuffer));

    WriteDataToBuffer (binaryBuffer, commandCode, sizeof (CommandCode));

    if (commandCode->arguments & REGISTER_ARGUMENT) {
        WriteDataToBuffer (binaryBuffer, &registerIndex, sizeof (char));

        char *registerName = NULL;

        #define REGISTER(NAME, INDEX)           \
                if (registerIndex == INDEX) {   \
                    registerName = #NAME;       \
                }

        #include "Registers.def"

        #undef REGISTER

        ON_DEBUG (printf ("%s (size = %lu) ", registerName, sizeof (char)));
    }

    if (commandCode->arguments & IMMED_ARGUMENT) {
        WriteDataToBuffer (binaryBuffer, &immedArgument, sizeof (elem_t));

        ON_DEBUG (printf ("%lf (size = %lu)", immedArgument, sizeof (elem_t)));
    }

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK) \
                if (commandCode->opcode == ((CommandCode) COMMAND_CODE).opcode)     \
                    ASSEMBLER_CALLBACK                                              \


    #include "Instructions.def"

    #undef INSTRUCTION
    #undef WriteDataToBuffer

    ON_DEBUG (printf ("\n"));

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
