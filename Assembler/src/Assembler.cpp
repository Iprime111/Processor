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
static ProcessorErrorCode GetInstructionConstantData (TextLine *line, const AssemblerInstruction **instruction, int *offset);
static ProcessorErrorCode GetInstructionArgumentsData (const AssemblerInstruction *instruction, TextLine *line, CommandCode *commandCode, char *registerIndex, elem_t *immedArgument);
static ProcessorErrorCode EmitInstruction (int outFileDescriptor, CommandCode *commandCode, char registerIndex, elem_t immedArgument);

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

    if ((errorCode = GetInstructionConstantData (line, &instruction, &argumentsOffset)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    printf ("Instruction found: %s. Arguments: ", instruction->instructionName);

    char registerIndex = -1;
    elem_t immedArgument    = {};
    CommandCode commandCode = {};

    if ((errorCode = GetInstructionArgumentsData (instruction, line, &commandCode, &registerIndex, &immedArgument)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    RETURN EmitInstruction (outFileDescriptor, &commandCode, registerIndex, immedArgument);
}

static ProcessorErrorCode GetInstructionConstantData (TextLine *line, const AssemblerInstruction **instruction, int *offset) {
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

    switch (argumentsCount) {
        case 2:
            if (sscanf (line->pointer + offset, "%*s r%cx+%lf", registerIndex, immedArgument) > 0) {
                commandCode->hasImmedArgument = true;
                commandCode->hasRegisterArgument = true;
                *registerIndex -= 'a';
            }else {
                RETURN TOO_FEW_ARGUMENTS;
            }
            break;

        case 0:
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%*s %lf", immedArgument) > 0){
                commandCode->hasImmedArgument = true;

            } else if (sscanf (line->pointer + offset, "%*s r%cx", registerIndex) > 0) {
                if (*registerIndex < 'a' || *registerIndex > 'd') {
                    RETURN TOO_FEW_ARGUMENTS;
                }

                *registerIndex -= 'a';
                commandCode->hasRegisterArgument = true;
            }else {

                RETURN TOO_FEW_ARGUMENTS;
            }
            break;

        default:
            RETURN TOO_MANY_ARGUMENTS;
            break;
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitInstruction (int outFileDescriptor, CommandCode *commandCode, char registerIndex, elem_t immedArgument) {
    PushLog (3);
    custom_assert (commandCode, pointer_is_null, WRONG_INSTRUCTION);

    #define WriteDataToBuffer(data, size)                           \
        do {                                                        \
            if (!WriteBuffer (outFileDescriptor, data, size)) {     \
                RETURN OUTPUT_FILE_ERROR;                           \
            }                                                       \
        } while (0)

    WriteDataToBuffer ((char *) commandCode, sizeof (char));

    if (commandCode->hasRegisterArgument) {
        WriteDataToBuffer ((char *) &registerIndex, sizeof (char));

        printf ("r%cx (size = %lu) ", registerIndex + 'a', sizeof (char));
    }

    if (commandCode->hasImmedArgument) {
        WriteDataToBuffer ((char *) &immedArgument, sizeof (elem_t));

        printf ("%lf (size = %lu)", immedArgument, sizeof (elem_t));
    }

    #define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)   \
                if (commandCode->opcode == OPCODE)                              \
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
