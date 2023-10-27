#include <cstdio>
#include <cstdlib>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include <SFML/System.hpp>

#include "AssemblyHeader.h"
#include "Buffer.h"
#include "Debugger.h"
#include "FileIO.h"
#include "GraphicsProvider.h"
#include "MessageHandler.h"
#include "SecureStack/SecureStack.h"
#include "SoftProcessor.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"
#include "DSLFunctions.h"

static DebuggerAction ExecuteProgram (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, TextBuffer *sourceText);

static ProcessorErrorCode ReadInstruction (SPU *spu, Buffer <DebugInfoChunk> *breakpointsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *sourceText, bool *doStep);
static ProcessorErrorCode ReadHeader      (SPU *spu, Header *readHeader);
static ProcessorErrorCode ReadDebugInfo   (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Header *header, char *sourcePath);

static ProcessorErrorCode GenerateDisassembly (TextBuffer *disassemblyText, FileBuffer *disassemblyBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, char *binaryFilepath);

static ProcessorErrorCode GetArguments (SPU *spu, const AssemblerInstruction *instruction, const CommandCode *commandCode, elem_t **argumentPointer);

ProcessorErrorCode LaunchProgram (SPU *spu, char *sourceFilename, char *binaryFilename, sf::Mutex *workMutex) {
  	PushLog (1);

	#define FreeDataAndReturnIfErrors(message, ...)									\
			do {																	\
				ProcessorErrorCode errorCode_ = __VA_ARGS__;						\
				if (errorCode_ != NO_PROCESSOR_ERRORS) {							\
					if (errorCode_ != PROCESSOR_HALT) {								\
						PrintErrorMessage (errorCode_, message, NULL, NULL, -1);	\
					}																\
					workMutex->lock ();												\
					spu->isWorking = false;											\
					workMutex->unlock ();											\
					StackDestruct_ (&spu->processorStack);							\
					StackDestruct_ (&spu->callStack);								\
					DestroyBuffer  (&debugInfoBuffer);								\
					free (spu->ram);												\
					DestroyFileBuffer (&sourceData);								\
					DestroyBuffer (&breakpointsBuffer);								\
					free (sourceText.lines);										\
					RETURN errorCode_;												\
				}																	\
			} while (0)

	workMutex->lock ();
	spu->isWorking = true;
	workMutex->unlock ();

	Buffer <DebugInfoChunk> debugInfoBuffer  = {0, 0, NULL};
	Buffer <DebugInfoChunk> breakpointsBuffer = {0, 0, NULL};
	Header header = {};
	TextBuffer sourceText = {};
	FileBuffer sourceData = {};

	StackInitDefault_ (&spu->processorStack);
	StackInitDefault_ (&spu->callStack);

	spu->ram = (elem_t *) calloc (VRAM_SIZE + RAM_SIZE, sizeof (elem_t));
	if (!spu->ram) {
		FreeDataAndReturnIfErrors ("Can not allocate ram arrray", NO_BUFFER);
	}

	PrintSuccessMessage ("Reading header...", NULL);

	FreeDataAndReturnIfErrors ("Error occuried while reading header", ReadHeader (spu, &header));

	FreeDataAndReturnIfErrors ("Error occuried while reading debug info", ReadDebugInfo (spu, &debugInfoBuffer, &header, sourceFilename));

	if (sourceFilename && IsDebugMode ())
		FreeDataAndReturnIfErrors ("Error occuried while reading source file", ReadSourceFile (&sourceData, &sourceText, sourceFilename));

	if (IsDebugMode () && (!sourceFilename || !header.hasDebugInfo)) {
		if (sourceFilename) {
			free (sourceText.lines);
			DestroyFileBuffer (&sourceData);
		}
		FreeDataAndReturnIfErrors ("Error occuried while generating disassembly",
										GenerateDisassembly (&sourceText, &sourceData, &debugInfoBuffer, binaryFilename));
	}

	if (IsDebugMode ()) {
		FreeDataAndReturnIfErrors ("Error occuried while initializing breakpoints buffer",
									InitBuffer (&breakpointsBuffer, DEFAULT_BREAKPOINTS_BUFFER_CAPACITY));

		InitDebugConsole ();

		if (DebugConsole (spu, &debugInfoBuffer, &breakpointsBuffer) == QUIT_PROGRAM) {
			FreeDataAndReturnIfErrors ("", PROCESSOR_HALT);
		}
	}

	PrintSuccessMessage ("Starting execution...", NULL);

	while (ExecuteProgram (spu, &debugInfoBuffer, &breakpointsBuffer, &sourceText) != QUIT_PROGRAM) {};

  	FreeDataAndReturnIfErrors ("", PROCESSOR_HALT);
  	RETURN NO_PROCESSOR_ERRORS;

	#undef FreeDataAndReturnIfErrors
}

static DebuggerAction ExecuteProgram (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, TextBuffer *sourceText) {
	PushLog (1);

	custom_assert (spu, 				  	pointer_is_null, QUIT_PROGRAM);
	custom_assert (debugInfoBuffer, 	  	pointer_is_null, QUIT_PROGRAM);
	custom_assert (breakpointsBuffer, 	  	pointer_is_null, QUIT_PROGRAM);

	spu->ip = 0;
	spu->callStack.size      = 0;
	spu->processorStack.size = 0;

	for (size_t ramIndex = 0; ramIndex < RAM_SIZE + VRAM_SIZE; ramIndex++) {
		spu->ram [ramIndex] = 0;
	}

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

	const char *DisassemblyPath = "./disassembly_autogenerated.tmp"; // TODO mangle file name

	char message [MAX_MESSAGE_LENGTH] = "";
	snprintf (message, MAX_MESSAGE_LENGTH, "No debug info has been found. Generating disassembly... (auto-generated file can be found in %s)", DisassemblyPath);
	PrintWarningMessage (NO_PROCESSOR_ERRORS, message, NULL, NULL, -1);

	pid_t childProcess = fork ();

	if (childProcess == -1) {
		ProgramErrorCheck (FORK_ERROR, "Error occuried while forking disassembly process");
	} else if (childProcess == 0) {

		char executablePath [FILENAME_MAX] = "";
		ssize_t readBytes = readlink ("/proc/self/exe", executablePath, FILENAME_MAX);
		if (readBytes >= 0) {
			executablePath [readBytes] = '\0';
		}

		dirname (executablePath);
		strcat (executablePath, "/Disassembler");

		execl (executablePath, "Disassembler", "-b", binaryFilepath, "-o", DisassemblyPath, NULL);
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
		debugInfo.line = (int) lineIndex + 1;

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

static ProcessorErrorCode ReadDebugInfo (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Header *header, char *sourcePath) {
	PushLog (2);

	custom_assert (spu, 			pointer_is_null, NO_PROCESSOR);
	custom_assert (debugInfoBuffer, pointer_is_null, NO_BUFFER);
	custom_assert (header, 			pointer_is_null, WRONG_HEADER);

	if (!header->hasDebugInfo) {
		RETURN NO_PROCESSOR_ERRORS;
	}

	ProgramErrorCheck (InitBuffer (debugInfoBuffer, header->commandsCount), "Error occuried while initializing debug info buffer");
	ReadArrayData (spu, debugInfoBuffer->data, header->commandsCount, DebugInfoChunk);

	if (sourcePath) {
		debugInfoBuffer->currentIndex = debugInfoBuffer->capacity;
	}

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

	ON_DEBUG (
        char message [MAX_MESSAGE_LENGTH] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
	)

	ProcessorErrorCode operationErrorCode = instruction->callbackFunction (spu, &commandCode, argumentPointer);

	if ((commandCode.arguments & MEMORY_ARGUMENT) && spu->graphicsEnabled) {
		ProgramErrorCheck(UpdateGraphics (spu, (size_t) (argumentPointer - spu->ram)), "Error occuried while updating graphics");
	}

	RETURN operationErrorCode;
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
		if (spu->frequencySleep > 0) {
			sf::sleep(sf::microseconds(spu->frequencySleep));
		}

		*argumentPointer = (spu->ram + (size_t) **argumentPointer);
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
