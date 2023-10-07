#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#include "Assembler.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "TextTypes.h"
#include "Stack/Stack.h"

static ProcessorErrorCode CompileLine (TextLine *line, int outFileDescriptor);
static ProcessorErrorCode WriteInstructionData (int outFileDescriptor, const AssemblerInstruction *instruction, TextLine *line, int initialOffset);
static ProcessorErrorCode GetInstructionData (TextLine *line, const AssemblerInstruction **instruction, int *offset);

static ssize_t CountWhitespaces (TextLine *line);
static ssize_t FindActualStringEnd (TextLine *line);
static ssize_t FindActualStringBegin (TextLine *line);

ProcessorErrorCode AssembleFile (TextBuffer *file, int outFileDescriptor) {
    PushLog (1);

    custom_assert (file, pointer_is_null, NO_BUFFER);
    custom_assert (file->lines, pointer_is_null, NO_BUFFER);

    for (size_t lineIndex = 0; lineIndex < file->line_count; lineIndex++) {
        ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;
        if ((errorCode = CompileLine (file->lines + lineIndex, outFileDescriptor)) != NO_PROCESSOR_ERRORS) {
            RETURN errorCode;
        }
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileLine (TextLine *line, int outFileDescriptor) {
    PushLog (2);
    custom_assert (line,          pointer_is_null, NO_BUFFER);
    custom_assert (line->pointer, pointer_is_null, NO_BUFFER);

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    // Get instruction basic data
    const AssemblerInstruction *instruction = NULL;
    int argumentsOffset = 0;

    if ((errorCode = GetInstructionData (line, &instruction, &argumentsOffset)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    printf ("Instruction found: %s. Arguments: ", instruction->instructionName);

    WriteInstructionData (outFileDescriptor, instruction, line, argumentsOffset);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode GetInstructionData (TextLine *line, const AssemblerInstruction **instruction, int *offset) {
    PushLog (3);

    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (line,        pointer_is_null, NO_BUFFER);

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    int sscanfResult = sscanf (line->pointer, "%s%n", sscanfBuffer, offset);

    if (sscanfResult <= 0 || sscanfResult == EOF) {
        RETURN NO_BUFFER;
    }

    *instruction = FindInstructionByName (sscanfBuffer);

    if (!*instruction) {
        RETURN WRONG_INSTRUCTION;
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteInstructionData (int outFileDescriptor, const AssemblerInstruction *instruction, TextLine *line, int initialOffset) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ssize_t argumentsCount = CountWhitespaces (line);

    if (argumentsCount < 0) {
        RETURN TOO_FEW_ARGUMENTS;
    }

    CommandCode commandCode {};
    commandCode.opcode = instruction->commandCode.opcode;

    int registerChar  = 0;
    int  readedSymbols = 0;
    elem_t immedArgument = 0;

    // TODO plus sign

    TextLine argsLine {line->pointer + initialOffset, line->length - (size_t) initialOffset};
    ssize_t offset = FindActualStringBegin (&argsLine) + initialOffset;

    switch (argumentsCount) {
        case 0:
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%lf", &immedArgument) == 1){
                commandCode.hasImmedArgument = true;

            } else if (sscanf (line->pointer + offset, " r%cx%n", &registerChar, &readedSymbols) > 0 && readedSymbols == 3) {
                if (registerChar < 'a' || registerChar > 'd') {
                    RETURN TOO_FEW_ARGUMENTS;
                }

                commandCode.hasRegisterArgument = true;

            }else {

                RETURN TOO_FEW_ARGUMENTS;
            }
            break;

        case 2:
            commandCode.hasImmedArgument = true;
            commandCode.hasRegisterArgument = true;
            break;

        default:
            RETURN TOO_MANY_ARGUMENTS;
            break;
    }

    #define WriteDataToBuffer(data, size)                           \
        do {                                                        \
            if (!WriteBuffer (outFileDescriptor, data, size)) {     \
                RETURN OUTPUT_FILE_ERROR;                           \
            }                                                       \
        } while (0)

    WriteDataToBuffer ((char *) &commandCode, sizeof (CommandCode));

    if (commandCode.hasRegisterArgument) {
        registerChar -= 'a';

        WriteDataToBuffer ((char *) &registerChar, sizeof (int));

        printf ("r%cx (size = %lu) ", registerChar + 'a', sizeof (int));
    }

    if (commandCode.hasImmedArgument) {
        WriteDataToBuffer ((char *) &immedArgument, sizeof (elem_t));

        printf ("%lf (size = %lu)", immedArgument, sizeof (elem_t));
    }

    #define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)   \
                if (commandCode.opcode = OPCODE)                                \
                    ASSEMBLER_CALLBACK                                          \


    #include "Instructions.def"

    #undef INSTRUCTION
    #undef WriteDataToBuffer

    printf ("\n");

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
        if (isspace (line->pointer [charIndex]))
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

    DetectWhitespacePosition (size_t charPointer = length; charPointer >= 0; charPointer--);
}

static ssize_t FindActualStringBegin (TextLine *line) {
    PushLog (3);

    DetectWhitespacePosition (size_t charPointer = 0; charPointer < line->length; charPointer++);
}

#undef DetectWhitespacePosition

// Instruction callback functions

#define INSTRUCTION(INSTRUCTION_NAME, INSTRUCTION_NUMBER, ARGUMENTS_COUNT, SCANF_SPECIFIERS, ...)   \
            INSTRUCTION_CALLBACK_FUNCTION (INSTRUCTION_NAME) {}

#include "Instructions.def"

#undef INSTRUCTION
