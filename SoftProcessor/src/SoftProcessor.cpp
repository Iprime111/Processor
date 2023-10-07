#include <cstdio>

#include "SoftProcessor.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"

#define CheckBuffer(spu)                                                                            \
            do {                                                                                    \
                custom_assert ((spu)->bytecode,                   pointer_is_null, NO_BUFFER);      \
                custom_assert ((spu)->bytecode->buffer_size >= 0, invalid_value,   BUFFER_ENDED);   \
                if ((size_t) (spu)->bytecode->buffer_size - sizeof (int) < spu->currentChar) {      \
                    RETURN BUFFER_ENDED;                                                            \
                }                                                                                   \
            }while (0)

#define PushValue(spu, value)                                                       \
            do {                                                                    \
                if (StackPush_ (&((spu)->processorStack), value) != NO_ERRORS) {    \
                    RETURN STACK_ERROR;                                             \
                }                                                                   \
            }while (0)

#define PopValue(spu, value)                                                        \
            do {                                                                    \
                if (StackPop_ (&((spu)->processorStack), value) != NO_ERRORS) {     \
                    RETURN STACK_ERROR;                                             \
                }                                                                   \
            }while (0)

#define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)                       \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                PushLog (3);                                                                    \
                do                                                                              \
                PROCESSOR_CALLBACK                                                              \
                while (0);                                                                      \
                RETURN NO_PROCESSOR_ERRORS;                                                     \
            }

#define ReadData(spu, destination, type) CopyVariableValue (destination, (spu)->bytecode->buffer + (spu)->currentChar, sizeof (type)); (spu)->currentChar += sizeof (type)


static ProcessorErrorCode ReadInstruction (SPU *spu);
static bool CopyVariableValue (void *destination, void *source, size_t size);

ProcessorErrorCode ExecuteFile (SPU *spu) {
    PushLog (1);

    spu->currentChar = 0;
    CheckBuffer (spu);



    StackInitDefault_ (&spu->processorStack);

    while (ReadInstruction(spu) == NO_PROCESSOR_ERRORS);

    StackDestruct_ (&spu->processorStack);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    printf ("Reading command: \n");

    CommandCode commandCode {};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByNumber (commandCode.opcode);

    if (instruction == NULL){
        RETURN WRONG_INSTRUCTION;
    }

    printf ("%s\n", instruction->instructionName);

    RETURN instruction->callbackFunction (spu, &commandCode);
}

static bool CopyVariableValue (void *destination, void *source, size_t size) {
    PushLog (4);

    custom_assert (destination, pointer_is_null, false);
    custom_assert (source,      pointer_is_null, false);

    if (destination == source) {
        RETURN true;
    }

    char *destinationPointer = (char *) destination;
    char *sourcePointer = (char *) source;

    for (size_t dataChunk = 0; dataChunk < size; dataChunk++) {
        *(destinationPointer + dataChunk) = *(sourcePointer + dataChunk);
    }

    RETURN true;
}


// Processor instructions

#include "Instructions.def"
