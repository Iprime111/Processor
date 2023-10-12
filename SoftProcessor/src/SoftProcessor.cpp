#include "MessageHandler.h"
#include "SoftProcessor.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"

#include <cstdio>

#define PushValue(spu, value)                                                       \
            do {                                                                    \
                if (StackPush_ (&((spu)->processorStack), value) != NO_ERRORS) {    \
                    PrintErrorMessage (STACK_ERROR, "Stack error occuried", NULL);  \
                    RETURN STACK_ERROR;                                             \
                }                                                                   \
            }while (0)

#define PopValue(spu, value)                                                        \
            do {                                                                    \
                if (StackPop_ (&((spu)->processorStack), value) != NO_ERRORS) {     \
                    PrintErrorMessage (STACK_ERROR, "Stack error occuried", NULL);  \
                    RETURN STACK_ERROR;                                             \
                }                                                                   \
            }while (0)

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)                 \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                PushLog (3);                                                                    \
                do                                                                              \
                PROCESSOR_CALLBACK                                                              \
                while (0);                                                                      \
                RETURN NO_PROCESSOR_ERRORS;                                                     \
            }

static ProcessorErrorCode ReadInstruction (SPU *spu);

// TODO add processor dump
ProcessorErrorCode ExecuteFile (SPU *spu) {
  	PushLog (1);

  	CheckBuffer (spu);

  	StackInitDefault_ (&spu->processorStack);

  	while (ReadInstruction (spu) == NO_PROCESSOR_ERRORS);

  	StackDestruct_ (&spu->processorStack);

  	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (SPU *spu) {
	PushLog (2);

	CheckBuffer (spu);

	ON_DEBUG (printf ("Reading command: "));

	CommandCode commandCode{0, 0};
	ReadData (spu, &commandCode, CommandCode);

	const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

	if (instruction == NULL) {
		ErrorFound (WRONG_INSTRUCTION, "Wrong instruction readed");
	}

	if ((~instruction->commandCode.arguments) & commandCode.arguments) {
		ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
	}

	ON_DEBUG (printf ("%s\n", instruction->instructionName));

	RETURN instruction->callbackFunction (spu, &commandCode);
}


// Processor instructions

#include "Instructions.def"
