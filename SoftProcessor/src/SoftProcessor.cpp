#include "SoftProcessor.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include <cstdio>

static FileBuffer *Bytecode;
static size_t CurrentChar = 0;
static Stack ProcessorStack = {};

#define CheckBuffer(fileBuffer)                                                                 \
            do {                                                                                \
                custom_assert (fileBuffer,                   pointer_is_null, NO_BUFFER);       \
                custom_assert ((fileBuffer)->buffer_size >= 0, invalid_value,   BUFFER_ENDED);  \
                if ((size_t) (fileBuffer)->buffer_size - sizeof (int) < CurrentChar) {          \
                    RETURN BUFFER_ENDED;                                                        \
                }                                                                               \
            }while (0)

#define PushValue(value)                                                \
            do {                                                        \
                if (StackPush_ (&ProcessorStack, value) != NO_ERRORS) { \
                    RETURN STACK_ERROR;                                 \
                }                                                       \
            }while (0)

#define PopValue(value)                                                 \
            do {                                                        \
                if (StackPop_ (&ProcessorStack, value) != NO_ERRORS) {  \
                    RETURN STACK_ERROR;                                 \
                }                                                       \
            }while (0)

#define INSTRUCTION(NAME, NUMBER, ARUMENTS_COUNT, SCANF_SPECIFIERS, PROCESSOR_CALLBACK, ...)    \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                PushLog (3);                                                                    \
                do                                                                              \
                PROCESSOR_CALLBACK                                                              \
                while (0);                                                                      \
                RETURN NO_PROCESSOR_ERRORS;                                                     \
            }


static ProcessorErrorCode ReadInstruction ();

ProcessorErrorCode ExecuteFile (FileBuffer *file) {
    PushLog (1);

    CurrentChar = 0;
    CheckBuffer (file);
    Bytecode = file;

    StackInitDefault_ (&ProcessorStack);

    while (ReadInstruction() == NO_PROCESSOR_ERRORS);

    StackDestruct_ (&ProcessorStack);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction () {
    PushLog (2);

    CheckBuffer (Bytecode);

    int instructionNumber = *(long long *) (Bytecode->buffer + CurrentChar);
    CurrentChar += sizeof (long long);

    const AssemblerInstruction *instruction = FindInstructionByNumber (instructionNumber);

    if (instruction == NULL){
        RETURN WRONG_INSTRUCTION;
    }

    RETURN instruction->callbackFunction ();
}

// Processor instructions

#include "Instructions.h"
