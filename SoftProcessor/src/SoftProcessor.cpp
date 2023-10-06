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
static elem_t RegisterValues [4] = {0, 0, 0, 0};

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

#define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)                       \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                PushLog (3);                                                                    \
                do                                                                              \
                PROCESSOR_CALLBACK                                                              \
                while (0);                                                                      \
                RETURN NO_PROCESSOR_ERRORS;                                                     \
            }

#define ReadData(type)  *(type *) (Bytecode->buffer + CurrentChar); CurrentChar += sizeof (type)


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

    CommandCode commandCode = ReadData (CommandCode);
    const AssemblerInstruction *instruction = FindInstructionByNumber (commandCode.opcode);

    if (instruction == NULL){
        RETURN WRONG_INSTRUCTION;
    }

    RETURN instruction->callbackFunction (&commandCode);
}

// Processor instructions

#include "Instructions.def"
