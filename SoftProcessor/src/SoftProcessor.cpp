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

static ProcessorErrorCode ReadInstruction (SPU *spu);

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

    printf ("Reading command: ");

    CommandCode commandCode {};
    ReadData (spu, &commandCode, char);

    const AssemblerInstruction *instruction = FindInstructionByNumber (commandCode.opcode);

    if (instruction == NULL){
        RETURN WRONG_INSTRUCTION;
    }

    printf ("%s\n", instruction->instructionName);

    RETURN instruction->callbackFunction (spu, &commandCode);
}


// Processor instructions

#include "Instructions.def"
