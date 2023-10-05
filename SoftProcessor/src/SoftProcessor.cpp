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

INSTRUCTION_CALLBACK_FUNCTION (push) {
    PushLog (3);

    CheckBuffer (Bytecode);

    elem_t value = *(elem_t *) (Bytecode->buffer + CurrentChar);
    CurrentChar += sizeof (elem_t);

    PushValue (value);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (out) {
    PushLog (3);

    elem_t value {};
    PopValue(&value);

    PrintData (CONSOLE_DEFAULT, CONSOLE_NORMAL, stdout, value);
    fputs ("\n", stdout);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (hlt) {
    PushLog (3);
    RETURN PROCESSOR_HALT;
}

INSTRUCTION_CALLBACK_FUNCTION (in) {
    PushLog (3);

    elem_t value {};

    printf_color (CONSOLE_WHITE, CONSOLE_BOLD, "Enter value: ");
    scanf ("%llu", &value);

    PushValue (value);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (add) {
    PushLog (3);

    elem_t value1 {}, value2{};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value1 + value2);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (sub) {
    PushLog (3);

    elem_t value1 {}, value2{};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value2 - value1);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (mul) {
    PushLog (3);

    elem_t value1 {}, value2{};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value1 * value2);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (div) {
    PushLog (3);

    elem_t value1 {}, value2{};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value2 / value1);

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (sin) {
    PushLog (3);

    elem_t value {};

    PopValue (&value);

    PushValue (sin (value));

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (cos) {
    PushLog (3);

    elem_t value {};

    PopValue (&value);

    PushValue (cos (value));

    RETURN NO_PROCESSOR_ERRORS;
}

INSTRUCTION_CALLBACK_FUNCTION (sqrt) {
    PushLog (3);

    elem_t value {};

    PopValue (&value);

    PushValue (sqrt (value));

    RETURN NO_PROCESSOR_ERRORS;
}
