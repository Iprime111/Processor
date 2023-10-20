#include "AssemblyHeader.h"
#include "MessageHandler.h"
#include "SecureStack/SecureStack.h"
#include "SoftProcessor.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"
#include "DSLFunctions.h"

#include <cstddef>
#include <stdio.h>
#include <string.h>

static ProcessorErrorCode ReadInstruction (SPU *spu);
static ProcessorErrorCode ReadHeader (SPU *spu);

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer);

// TODO add processor dump
ProcessorErrorCode ExecuteFile (SPU *spu) {
  	PushLog (1);

  	CheckBuffer (spu);

	PrintSuccessMessage ("Reading header...", NULL);
	ReadHeader (spu);

  	StackInitDefault_ (&spu->processorStack);
	PrintSuccessMessage ("Starting execution...", NULL);

  	while (ReadInstruction (spu) == NO_PROCESSOR_ERRORS);

  	StackDestruct_ (&spu->processorStack);

  	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadHeader (SPU *spu) {
	PushLog (2);

	custom_assert (spu, pointer_is_null, NO_PROCESSOR);

	Header mainHeader {};
    Header readedHeader {};
    InitHeader (&mainHeader);

    ReadData (spu, &readedHeader, Header);

    #define CheckHeaderField(field, predicate)                                      \
                if (!(predicate)) {                                                 \
                    ErrorFoundInProgram (WRONG_HEADER, "Header field " #field " is wrong");  \
                }

    CheckHeaderField (SIGNATURE,  readedHeader.signature == mainHeader.signature);
    CheckHeaderField (VERSION,    !strcmp (readedHeader.version,   mainHeader.version));
    CheckHeaderField (BYTE_ORDER, !strcmp (readedHeader.byteOrder, mainHeader.byteOrder));

    #undef CheckHeaderField

	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (SPU *spu) {
	PushLog (2);

	CheckBuffer (spu);

	CommandCode commandCode{0, 0};
	ReadData (spu, &commandCode, CommandCode);

	const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

	if (instruction == NULL) {
		ErrorFoundInProgram (WRONG_INSTRUCTION, "Wrong instruction readed");
	}

	elem_t *argumentPointer = NULL;

	GetArguments (spu, instruction, &commandCode, &argumentPointer);

	#ifndef _NDEBUG
        char message [128] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
    #endif

	RETURN instruction->callbackFunction (spu, &commandCode, argumentPointer);
}

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer) {
	PushLog (2);

	if ((~instruction->commandCode.arguments) & commandCode->arguments) {
		ErrorFoundInProgram (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
	}

	if (commandCode->arguments == NO_ARGUMENTS) {
		RETURN NO_PROCESSOR_ERRORS;
	}

	elem_t immedArgument = NAN;
	unsigned char registerIndex = 0;

	if (commandCode->arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT) || commandCode->arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT | MEMORY_ARGUMENT)) {
		ReadData (spu, &registerIndex, unsigned char);
		ReadData (spu, &immedArgument, elem_t);

		spu->tmpArgument = spu->registerValues [registerIndex] + immedArgument;

		*argumentPointer = &spu->tmpArgument;
	}else if (commandCode->arguments & REGISTER_ARGUMENT) {
		ReadData (spu, &registerIndex, unsigned char);

		*argumentPointer = (spu->registerValues + registerIndex);
	} else if (commandCode->arguments & IMMED_ARGUMENT) {
		ReadData (spu, &spu->tmpArgument, elem_t);

		*argumentPointer = &spu->tmpArgument;
	}

	if (commandCode->arguments & MEMORY_ARGUMENT) {
		*argumentPointer = (spu->ram + (ssize_t) **argumentPointer);
	}

	RETURN NO_PROCESSOR_ERRORS;
}

// Processor instructions

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ...) 	\
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {               	\
                PushLog (3);                                     	\
                CheckBuffer (spu);                               	\
                do                                               	\
                PROCESSOR_CALLBACK                               	\
                while (0);                                       	\
                RETURN NO_PROCESSOR_ERRORS;                      	\
            }


#include "Instructions.def"

#undef INSTRUCTION
