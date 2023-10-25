#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
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

static DebuggerAction ExecuteProgram (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, TextBuffer *sourceText);

static ProcessorErrorCode ReadInstruction (SPU *spu, Buffer <DebugInfoChunk> *breakpointsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *sourceText, bool *doStep);
static ProcessorErrorCode ReadHeader      (SPU *spu, Header *readHeader);
static ProcessorErrorCode ReadDebugInfo   (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Header *header);

static ProcessorErrorCode GenerateDisassembly (TextBuffer *disassemblyText, FileBuffer *disassemblyBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, char *binaryFilepath);

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer);

ProcessorErrorCode LaunchProgram (SPU *spu, char *sourceFilename, char *binaryFilename) {
  	PushLog (1);

	StackInitDefault_ (&spu->processorStack);
	StackInitDefault_ (&spu->callStack);

	PrintSuccessMessage ("Reading header...", NULL);

	Header header = {};
	ProgramErrorCheck (ReadHeader (spu, &header), "Error occuried while reading header");

	Buffer <DebugInfoChunk> debugInfoBuffer = {0, 0, NULL};
	ProgramErrorCheck (ReadDebugInfo (spu, &debugInfoBuffer, &header), "Error occuried while reading debug info");

	TextBuffer sourceText = {};
	FileBuffer sourceData = {};

	if (sourceFilename && IsDebugMode ())
		ProgramErrorCheck (ReadSourceFile (&sourceData, &sourceText, sourceFilename), "Error occuried while reading source file");

	if (IsDebugMode () && (!sourceFilename || !header.hasDebugInfo))
		ProgramErrorCheck (GenerateDisassembly (&sourceText, &sourceData, &debugInfoBuffer, binaryFilename), "Error occuried while generating disassembly");

	Buffer <DebugInfoChunk> breakpointsBuffer = {0, 0, NULL};
	if (IsDebugMode ()) {
		ProgramErrorCheck(InitBuffer (&breakpointsBuffer, DEFAULT_BREAKPOINTS_BUFFER_CAPACITY), "Error occuried while initializing breakpoints buffer");
		InitDebugConsole ();

		DebugConsole (spu, &debugInfoBuffer, &breakpointsBuffer);
	}

	PrintSuccessMessage ("Starting execution...", NULL);

	while (ExecuteProgram (spu, &debugInfoBuffer, &breakpointsBuffer, &sourceText) != QUIT_PROGRAM) {};

  	StackDestruct_ (&spu->processorStack);
	StackDestruct_ (&spu->callStack);

	DestroyBuffer  (&debugInfoBuffer);

	if (IsDebugMode ()) {
		DestroyBuffer     (&breakpointsBuffer);
		DestroyFileBuffer (&sourceData);
		free (sourceText.lines);
	}

  	RETURN NO_PROCESSOR_ERRORS;
}

static DebuggerAction ExecuteProgram (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, TextBuffer *sourceText) {
	PushLog (1);

	custom_assert (spu, 				  	pointer_is_null, QUIT_PROGRAM);
	custom_assert (debugInfoBuffer, 	  	pointer_is_null, QUIT_PROGRAM);
	custom_assert (breakpointsBuffer, 	  	pointer_is_null, QUIT_PROGRAM);

	spu->ip = 0;
	spu->callStack.size      = 0;
	spu->processorStack.size = 0;

	bool doStep = false;
	ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

	while ((errorCode = ReadInstruction (spu, breakpointsBuffer, debugInfoBuffer, sourceText, &doStep)) == NO_PROCESSOR_ERRORS) {};

	if (errorCode == PROCESSOR_HALT) {
		RETURN QUIT_PROGRAM;
	} else if (errorCode == RESET_PROCESSOR) {
		RETURN RUN_PROGRAM;
	}

	RETURN QUIT_PROGRAM;
}

static ProcessorErrorCode GenerateDisassembly (TextBuffer *disassemblyText, FileBuffer *disassemblyBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, char *binaryFilepath) {
	PushLog (2);

	const char *DisassemblyPath = "./disassembly_autogenerated.tmp";

	char message [MAX_MESSAGE_LENGTH] = "";
	snprintf (message, MAX_MESSAGE_LENGTH, "No debug info has been found. Generating disassembly... (auto-generated file can be found in %s)", DisassemblyPath);
	PrintWarningMessage (NO_PROCESSOR_ERRORS, message, NULL, NULL, -1);

	pid_t childProcess = fork ();

	if (childProcess == -1) {
		ProgramErrorCheck (FORK_ERROR, "Error occuried while forking disassembly process");
	} else if (childProcess == 0) {
		execl ("./bin/Disassembler", "Disassembler", "-b", binaryFilepath, "-o", DisassemblyPath, NULL);
        exit (0);
	}

    int status = 0;
    wait (&status);

	ReadSourceFile (disassemblyBuffer, disassemblyText, DisassemblyPath);

	bool hasDisassemblyStarted = false;

	for (size_t lineIndex = 0; lineIndex < disassemblyText->line_count; lineIndex++) {
		if (!hasDisassemblyStarted) {
			if (strstr (disassemblyText->lines [lineIndex].pointer, "ip")) {
				hasDisassemblyStarted = true;
			}
			continue;
		}

		DebugInfoChunk debugInfo = {};
		int addressLength = 0;

		if (sscanf (disassemblyText->lines [lineIndex].pointer, "%lu%n", &debugInfo.address, &addressLength) <= 0) {
			break;
		}

		disassemblyText->lines [lineIndex].pointer += addressLength;
		debugInfo.line = (int) lineIndex;

		WriteDataToBufferErrorCheck ("Error occuried while writing debug info to a buffer", debugInfoBuffer, &debugInfo, 1);
	}

	RETURN NO_PROCESSOR_ERRORS;
}


static ProcessorErrorCode ReadHeader (SPU *spu, Header *readHeader) {
	PushLog (2);

	custom_assert (spu, 	   pointer_is_null, NO_PROCESSOR);
	custom_assert (readHeader, pointer_is_null, WRONG_HEADER);

	Header mainHeader {};
    InitHeader (&mainHeader);
    ReadData (spu, readHeader, Header);

	ShrinkBytecodeBuffer (spu, spu->ip);

	RETURN CheckHeader (readHeader);
}

static ProcessorErrorCode ReadDebugInfo (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Header *header) {
	PushLog (2);

	custom_assert (spu, 			pointer_is_null, NO_PROCESSOR);
	custom_assert (debugInfoBuffer, pointer_is_null, NO_BUFFER);
	custom_assert (header, 			pointer_is_null, WRONG_HEADER);

	if (!header->hasDebugInfo && IsDebugMode ()) {
		RETURN NO_PROCESSOR_ERRORS;
	}

	ProgramErrorCheck (InitBuffer (debugInfoBuffer, header->commandsCount), "Error occuried while initializing debug info buffer");
	ReadArrayData (spu, debugInfoBuffer->data, header->commandsCount, DebugInfoChunk);
	debugInfoBuffer->currentIndex = debugInfoBuffer->capacity;

	ShrinkBytecodeBuffer (spu, spu->ip);

	RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (SPU *spu, Buffer <DebugInfoChunk> *breakpointsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *sourceText, bool *doStep) {
	PushLog (2);

	CheckBuffer (spu);

	if (IsDebugMode ()) {
		DebugInfoChunk breakpointByAddress = {spu->ip, -1};
		DebugInfoChunk *foundBreakpoint = FindValueInBuffer (breakpointsBuffer, &breakpointByAddress, DebugInfoChunkComparatorByAddress);
		if (foundBreakpoint || *doStep) {
			*doStep = false;

			if (!foundBreakpoint)
				foundBreakpoint = FindValueInBuffer (debugInfoBuffer, &breakpointByAddress, DebugInfoChunkComparatorByAddress);

			switch (BreakpointStop (spu, debugInfoBuffer, breakpointsBuffer, foundBreakpoint, sourceText)) {
				case STEP_PROGRAM:
					*doStep = true;
					break;

				case QUIT_PROGRAM:
					RETURN PROCESSOR_HALT;
					break;

				case RUN_PROGRAM:
					RETURN RESET_PROCESSOR;
					break;

				case CONTINUE_PROGRAM:
					break;

				default:
					break;
			};
		}
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
