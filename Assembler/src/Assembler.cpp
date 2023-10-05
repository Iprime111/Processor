#include <cstring>
#include <ctype.h>
#include <stdio.h>


#include "Assembler.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "TextTypes.h"
#include "Stack/Stack.h"

static ProcessorErrorCode CompileLine (TextLine *line, int outFileDescriptor);
static ProcessorErrorCode CheckArgumentsCount (TextLine *line, const AssemblerInstruction *instruction);
static ProcessorErrorCode WriteArguments (int outFileDescriptor, const AssemblerInstruction *instruction, TextLine *line, int initialOffset);
static ssize_t CountWhitespaces (TextLine *line);
static ssize_t FindActualStringEnd (TextLine *line);

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

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    int offset = 0;
    int sscanfResult = sscanf (line->pointer, "%s%n", sscanfBuffer, &offset);

    if (sscanfResult <= 0 || sscanfResult == EOF) {
        RETURN NO_BUFFER;
    }

    const AssemblerInstruction *instruction = FindInstructionByName (sscanfBuffer);

    if (instruction == NULL) {
        RETURN WRONG_INSTRUCTION;
    }

    printf ("Instruction found: %s. Arguments: ", instruction->instructionName);
    if (!WriteBuffer (outFileDescriptor, (const char *) &instruction->instruction, sizeof (long long))) {
        RETURN OUTPUT_FILE_ERROR;
    }

    RETURN WriteArguments (outFileDescriptor, instruction, line, offset);
}

static ProcessorErrorCode CheckArgumentsCount (TextLine *line, const AssemblerInstruction *instruction) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ssize_t whitespaceCount = CountWhitespaces (line);

    if (whitespaceCount < instruction->argumentsCount) {
        RETURN TOO_FEW_ARGUMENTS;
    }

    if (whitespaceCount > instruction->argumentsCount) {
        RETURN TOO_MANY_ARGUMENTS;
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteArguments (int outFileDescriptor, const AssemblerInstruction *instruction, TextLine *line, int initialOffset) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ProcessorErrorCode errorCode = CheckArgumentsCount (line, instruction);

    if (errorCode != NO_PROCESSOR_ERRORS){
        RETURN errorCode;
    }

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    for (size_t argumentIndex = 0; argumentIndex < instruction->argumentsCount; argumentIndex++) {
        int currentOffset =0;

        char printfSpecifier [8] = "";
        strcat (printfSpecifier, instruction->argumentsScanfSpecifiers [argumentIndex]);
        strcat (printfSpecifier, "%n");

        if (sscanf (line->pointer + initialOffset, printfSpecifier, sscanfBuffer, &initialOffset) != 1) {
            RETURN TOO_FEW_ARGUMENTS;
        }

        initialOffset += currentOffset;

        if (!WriteBuffer (outFileDescriptor, sscanfBuffer, sizeof (elem_t))) {
            RETURN OUTPUT_FILE_ERROR;
        }

        printf (instruction->argumentsScanfSpecifiers [argumentIndex],  *(elem_t *) sscanfBuffer);
        printf (" (size: %lu), ", sizeof (elem_t));
    }

    printf ("\n");

    RETURN NO_PROCESSOR_ERRORS;
}

static ssize_t CountWhitespaces (TextLine *line) {
    PushLog (3);

    custom_assert (line,          pointer_is_null, -1);
    custom_assert (line->pointer, pointer_is_null, -1);

    ssize_t lineLength = FindActualStringEnd (line);
    ssize_t spaceCount = 0;

    for (int charIndex = 0; charIndex < lineLength; charIndex++) {
        if (isspace (line->pointer [charIndex]))
            spaceCount++;
    }

    RETURN spaceCount;
}

static ssize_t FindActualStringEnd (TextLine *line) {
    PushLog (3);

    custom_assert (line,          pointer_is_null, -1);
    custom_assert (line->pointer, pointer_is_null, -1);

    for (size_t charPointer = line->length; charPointer >= 0; charPointer--) {
        if (!isspace (line->pointer [charPointer])){
            RETURN (ssize_t) charPointer;
        }
    }

    RETURN 0;
}


INSTRUCTION_CALLBACK_FUNCTION (in) {}
INSTRUCTION_CALLBACK_FUNCTION (hlt) {}
INSTRUCTION_CALLBACK_FUNCTION (out) {}
INSTRUCTION_CALLBACK_FUNCTION (push) {}
INSTRUCTION_CALLBACK_FUNCTION (add) {}
INSTRUCTION_CALLBACK_FUNCTION (sub) {}
INSTRUCTION_CALLBACK_FUNCTION (mul) {}
INSTRUCTION_CALLBACK_FUNCTION (div) {}
INSTRUCTION_CALLBACK_FUNCTION (sin) {}
INSTRUCTION_CALLBACK_FUNCTION (cos) {}
INSTRUCTION_CALLBACK_FUNCTION (sqrt) {}
