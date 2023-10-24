#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "AssemblyHeader.h"
#include "Buffer.h"
#include "Debugger.h"
#include "FileIO.h"
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

static ProcessorErrorCode ReadInstruction (SPU *spu, Buffer <DebugInfoChunk> *breakpointsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *sourceText);
static ProcessorErrorCode ReadHeader      (SPU *spu, Header *readHeader);
static ProcessorErrorCode ReadDebugInfo   (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, size_t commandsCount);

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer);

//TODO debug mode turn off function
ProcessorErrorCode ExecuteFile (SPU *spu, char *sourceFilename) {
  	PushLog (1);

	TextBuffer sourceText = {};
	FileBuffer sourceData = {};
	ProgramErrorCheck (ReadSourceFile (&sourceData, &sourceText, sourceFilename), "Error occuried while reading source file");

  	CheckBuffer (spu);

	PrintSuccessMessage ("Reading header...", NULL);

	Header header = {};
	ProgramErrorCheck (ReadHeader (spu, &header), "Error occuried while reading header");

	Buffer <DebugInfoChunk> debugInfoBuffer =   {0, 0, NULL};
	ProgramErrorCheck (ReadDebugInfo (spu, &debugInfoBuffer, header.commandsCount), "Error occuried while reading debug info");

	Buffer <DebugInfoChunk> breakpointsBuffer = {0, 0, NULL};
	ProgramErrorCheck(InitBuffer (&breakpointsBuffer, DEFAULT_BREAKPOINTS_BUFFER_CAPACITY), "Error occuried while initializing breakpoints buffer");
	InitDebugConsole ();

	DebugConsole (spu, &debugInfoBuffer, &breakpointsBuffer);

  	StackInitDefault_ (&spu->processorStack);
	StackInitDefault_ (&spu->callStack);
	PrintSuccessMessage ("Starting execution...", NULL);

  	while (ReadInstruction (spu, &breakpointsBuffer, &debugInfoBuffer, &sourceText) == NO_PROCESSOR_ERRORS);

  	StackDestruct_ (&spu->processorStack);
	StackDestruct_ (&spu->callStack);

	DestroyBuffer (&debugInfoBuffer);
	DestroyBuffer (&breakpointsBuffer);

	DestroyFileBuffer (&sourceData);
	free (sourceText.lines);

  	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadHeader (SPU *spu, Header *readHeader) {
	PushLog (2);

	custom_assert (spu, pointer_is_null, NO_PROCESSOR);

	Header mainHeader {};
    InitHeader (&mainHeader);
    ReadData (spu, readHeader, Header);

	ShrinkBytecodeBuffer (spu, spu->ip);

	RETURN CheckHeader (readHeader);
}

static ProcessorErrorCode ReadDebugInfo (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, size_t commandsCount) {
	PushLog (2);

	ProgramErrorCheck (InitBuffer (debugInfoBuffer, commandsCount), "Error occuried while initializing debug info buffer");
	ReadArrayData (spu, debugInfoBuffer->data, commandsCount, DebugInfoChunk);
	debugInfoBuffer->currentIndex = debugInfoBuffer->capacity;

	spu->bytecode.buffer      +=           spu->ip;
	spu->bytecode.buffer_size -= (ssize_t) spu->ip;
	spu->ip = 0;

	ShrinkBytecodeBuffer (spu, spu->ip);

	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (SPU *spu, Buffer <DebugInfoChunk> *breakpointsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *sourceText) {
	PushLog (2);

	CheckBuffer (spu);

	DebugInfoChunk breakpointByAddress = {spu->ip, -1};
	const DebugInfoChunk *foundBreakpoint = FindValueInBuffer (breakpointsBuffer, &breakpointByAddress, DebugInfoChunkComparatorByAddress);
	if (foundBreakpoint) {
		BreakpointStop (spu, debugInfoBuffer, breakpointsBuffer, foundBreakpoint, sourceText);
	}

	CommandCode commandCode{0, 0};
	ReadData (spu, &commandCode, CommandCode);

	const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

	if (instruction == NULL) {
		ProgramErrorCheck (WRONG_INSTRUCTION, "Wrong instruction readed");
	}

	elem_t *argumentPointer = NULL;

	GetArguments (spu, instruction, &commandCode, &argumentPointer);

	#ifndef _NDEBUG
        char message [MAX_MESSAGE_LENGTH] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
    #endif

	RETURN instruction->callbackFunction (spu, &commandCode, argumentPointer);
}

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer) {
	PushLog (2);

	if ((~instruction->commandCode.arguments) & commandCode->arguments) {
		ProgramErrorCheck (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
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
		usleep (spu->frequencySleep);

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
