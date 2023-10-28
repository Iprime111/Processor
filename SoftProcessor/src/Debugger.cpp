#include <cstddef>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

#undef RETURN

#include "Buffer.h"
#include "CommonModules.h"
#include "Debugger.h"
#include "ColorConsole.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "Registers.h"
#include "SPU.h"
#include "StringProcessing.h"
#include "TextTypes.h"

static ProcessorErrorCode PlaceBreakpoint  (SPU *spu, char *arguments, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer);
static ProcessorErrorCode PrintSpuData     (SPU *spu, char *arguments);

static void DumpBytecode (SPU *spu, char *arguments);
static void DumpMemory   (SPU *spu, char *arguments);
static ProcessorErrorCode GetDumpArguments (char *arguments, ssize_t *dumpAddress, ssize_t *dumpSize, ssize_t maxSize);

static void PrintMemoryValue      (SPU *spu, ssize_t address, char *arguments);
static void DumpMemory            (SPU *spu, char *arguments);
static void PrintRegister         (SPU *spu, unsigned char registerIndex, char *registerName);
static void PrintRegisterAndImmed (SPU *spu, unsigned char registerIndex, elem_t value);
static void PrintImmed            (elem_t value);

static bool HasRamBrackets (TextLine *arguments);
static void ShowDebuggerLogo (FILE *stream);

static char DEBUGGER_ERROR_PREFIX [] = "Debugger";

ProcessorErrorCode InitDebugConsole () {
    PushLog (3);

    //fprintf (stderr, CLEAR_SCREEN());
    ShowDebuggerLogo (stderr);

    RETURN NO_PROCESSOR_ERRORS;
}

DebuggerAction BreakpointStop (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, const DebugInfoChunk *breakpointData, TextBuffer *text) {
    PushLog (2);

    fprintf (stderr, "Break: " BOLD_WHITE_COLOR "%s\n" WHITE_COLOR "Ip:     " BOLD_WHITE_COLOR "%lu\n", text->lines [breakpointData->line - 1].pointer, breakpointData->address);

    RETURN DebugConsole (spu, debugInfoBuffer, breakpointsBuffer);
}

DebuggerAction DebugConsole (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer) {
    PushLog (2);

    #define DestroyBufferAndReturn(returnValue) \
            free (input);                       \
            RETURN returnValue;                 \

    while (true) {
        char *input = readline (WHITE_COLOR "Assembly debugger" BOLD_WHITE_COLOR " # " WHITE_COLOR);

        add_history (input);

        char commandName [MAX_DEBUGGER_COMMAND_LENGTH] = "";
        int commandNameLength = 0;
        sscanf (input, "%s%n", commandName, &commandNameLength);

        char *argumentsLine = input + commandNameLength;

        #define DEBUGGER_COMMAND_(name, shortName, ...)                               \
            if (!strcmp (commandName, name) || !strcmp (commandName, shortName)) {    \
                __VA_ARGS__;                                                          \
            }                                                                         

        DEBUGGER_COMMAND_ ("quit",       "q",  DestroyBufferAndReturn (QUIT_PROGRAM));
        DEBUGGER_COMMAND_ ("continue",   "c",  DestroyBufferAndReturn (CONTINUE_PROGRAM));
        DEBUGGER_COMMAND_ ("run",        "r",  DestroyBufferAndReturn (RUN_PROGRAM));
        DEBUGGER_COMMAND_ ("step",       "s",  DestroyBufferAndReturn (STEP_PROGRAM));
        DEBUGGER_COMMAND_ ("print",      "p",  {PrintSpuData    (spu, argumentsLine);                                     free (input); continue;});
        DEBUGGER_COMMAND_ ("breakpoint", "b",  {PlaceBreakpoint (spu, argumentsLine, debugInfoBuffer, breakpointsBuffer); free (input); continue;});
        DEBUGGER_COMMAND_ ("memory",     "m",  {DumpMemory      (spu, argumentsLine);                                     free (input); continue;});
        DEBUGGER_COMMAND_ ("bytecode",   "by", {DumpBytecode    (spu, argumentsLine);                                     free (input); continue;});
        DEBUGGER_COMMAND_ ("telescope",  "t",  {DumpStackData   (&spu->processorStack);                                   free (input); continue;});


        PrintErrorMessage (NO_PROCESSOR_ERRORS, "Please enter valid command", DEBUGGER_ERROR_PREFIX, NULL, -1);

        #undef DEBUGGER_COMMAND_

        free (input);
        #undef DestroyBufferAndReturn
    }


}

ProcessorErrorCode ReadSourceFile (FileBuffer *fileBuffer, TextBuffer *text, const char *filename) {
    PushLog (3);
    
    // TODO
    // #define check_err(func, msg)
        // if (!func)
            // ProgramErrorCheck (NO_BUFFER, msg);
    // 
    // check_err(CreateFileBuffer (fileBuffer, filename), "Error occuried while creating source file buffer")
             
	if (!CreateFileBuffer (fileBuffer, filename)) {
        ProgramErrorCheck (NO_BUFFER, "Error occuried while creating source file buffer");
    }

    if (!ReadFileLines (filename, fileBuffer, text)) {
        ProgramErrorCheck (NO_BUFFER, "Error occuried while reading file lines");
    }

    if (!ChangeNewLinesToZeroes (text)) {
        ProgramErrorCheck (NO_BUFFER, "Error occuried while changing new line symbols to zero symbols");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static void DumpBytecode (SPU *spu, char *arguments) {

    PushLog (3);

    custom_assert (arguments, pointer_is_null, (void)0);

    ssize_t dumpAddress = 0;
    ssize_t dumpSize    = 0;


    if (GetDumpArguments (arguments, &dumpAddress, &dumpSize, spu->bytecode.buffer_size) != NO_PROCESSOR_ERRORS) {
        RETURN;
    }

    fputs ("\n\n\n" MOVE_CURSOR_UP (3), stderr);

    const ssize_t TitleFieldSize = 10;
    fprintf_color (CONSOLE_YELLOW, CONSOLE_BOLD, stderr, "\rADDRESS:" MOVE_CURSOR_DOWN (1) "\rVALUE:" MOVE_CURSOR_UP (1));

    for (ssize_t byteIndex = dumpAddress; byteIndex < dumpAddress + dumpSize; byteIndex++) {
        const ssize_t NumberFieldSize = 8;
        ssize_t offset = (byteIndex - dumpAddress) * NumberFieldSize + TitleFieldSize;

        if (spu->ip == byteIndex) {
            fprintf_color (CONSOLE_BLUE, CONSOLE_BOLD, stderr, "\r" MOVE_CURSOR_DOWN (2) MOVE_CURSOR_FORWARD (%ld) "^" MOVE_CURSOR_UP (2), offset);
        }

        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "\r" MOVE_CURSOR_FORWARD (%ld) "%-7ld" MOVE_CURSOR_DOWN (1) "\r" MOVE_CURSOR_FORWARD (%ld) "%-5ld" MOVE_CURSOR_UP (1),
                            offset, byteIndex, offset, spu->bytecode.buffer [byteIndex]);
    }

    fputs ("\n\n\n\r", stderr);

    RETURN;
}

static void DumpMemory (SPU *spu, char *arguments) {
    PushLog (3);

    custom_assert (arguments, pointer_is_null, (void)0);

    ssize_t dumpAddress = 0;
    ssize_t dumpSize    = 0;

    if (GetDumpArguments (arguments, &dumpAddress, &dumpSize, (ssize_t) (RAM_SIZE + VRAM_SIZE)) != NO_PROCESSOR_ERRORS) {
        RETURN;
    }

    fputs ("\n\n" MOVE_CURSOR_UP (1), stderr);

    const ssize_t TitleFieldSize = 10;
    fprintf_color (CONSOLE_YELLOW, CONSOLE_BOLD, stderr, "\rADDRESS:" MOVE_CURSOR_DOWN (1) "\rVALUE:" MOVE_CURSOR_UP (1));

    for (ssize_t ramIndex = dumpAddress; ramIndex < dumpAddress + dumpSize; ramIndex++) {
        const ssize_t NumberFieldSize = 8;
        ssize_t offset = (ramIndex - dumpAddress) * NumberFieldSize + TitleFieldSize;

        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "\r" MOVE_CURSOR_FORWARD (%ld) "%-7ld" MOVE_CURSOR_DOWN (1) "\r" MOVE_CURSOR_FORWARD (%ld) "%-5.g" MOVE_CURSOR_UP (1),
                            offset, ramIndex, offset, spu->ram [ramIndex]);
    }

    fputs ("\n\n\r", stderr);

    RETURN;
}

static ProcessorErrorCode GetDumpArguments (char *arguments, ssize_t *dumpAddress, ssize_t *dumpSize, ssize_t maxSize) {
    PushLog (4);

    if (sscanf(arguments, "%ld %ld", dumpAddress, dumpSize) < 2) {
        PrintErrorMessage (NO_PROCESSOR_ERRORS, "Incorrect arguments", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN TOO_FEW_ARGUMENTS;
    }

    if (*dumpAddress < 0 || *dumpAddress >= maxSize) {
        PrintErrorMessage (NO_PROCESSOR_ERRORS, "Incorrect dump address", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN TOO_FEW_ARGUMENTS;
    }

    if (*dumpSize < 0 || *dumpAddress + *dumpSize > maxSize) {
        PrintErrorMessage (NO_PROCESSOR_ERRORS, "Incorrect dump size", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN TOO_FEW_ARGUMENTS;
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode PrintSpuData (SPU *spu, char *arguments) {
    PushLog (3);

    arguments = strtok (arguments, " ");
    TextLine argumentsLine = {arguments, strlen (arguments)};

    bool isRamValue = HasRamBrackets (&argumentsLine);
    if (isRamValue) {
        argumentsLine.length--;
        argumentsLine.pointer [argumentsLine.length] = '\0';
        argumentsLine.pointer++;
    }

    #define FIND_REGISTER()                                                                                     \
            const Register *foundRegister = FindRegisterByName (registerName);                                  \
            if (foundRegister) {                                                                                \
                registerIndex = foundRegister->index;                                                           \
            } else {                                                                                            \
                PrintErrorMessage (NO_PROCESSOR_ERRORS, "Wrong register name", DEBUGGER_ERROR_PREFIX, NULL, -1);\
                RETURN TOO_FEW_ARGUMENTS;                                                                       \
            }

    elem_t value = 0;
    unsigned char registerIndex = 0;
    char registerName [MAX_REGISTER_NAME_LENGTH + 1] = "";

    if (sscanf (argumentsLine.pointer, "%3s+%lf", registerName, &value) == 2) {
        FIND_REGISTER ();

        if (!isRamValue)
            PrintRegisterAndImmed (spu, registerIndex, value);
        else
            value += spu->registerValues [registerIndex];

    } else if (sscanf (argumentsLine.pointer, "%lf", &value) > 0) {
        if (!isRamValue)
            PrintImmed (value);

    } else if (sscanf (argumentsLine.pointer, "%3s", registerName) > 0) {
        FIND_REGISTER ();

        if (!isRamValue)
            PrintRegister (spu, registerIndex, registerName);
        else
            value = spu->registerValues [registerIndex];

    } else {
        PrintErrorMessage (NO_PROCESSOR_ERRORS, "Wrong arguments prompt", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN TOO_FEW_ARGUMENTS;
    }

    if (isRamValue) {
        PrintMemoryValue (spu, (ssize_t) value, argumentsLine.pointer);
    }

    fprintf (stderr, "\n");

    RETURN NO_PROCESSOR_ERRORS;
}

static void PrintMemoryValue (SPU *spu, ssize_t address, char *arguments) {
    if (address >= (ssize_t) (RAM_SIZE + VRAM_SIZE) || address < 0) {
        fprintf (stderr, "Invalid address");
        return;
    }

    fprintf (stderr, "[%s]: %lf", arguments, spu->ram [address]);
}

static void PrintRegister (SPU *spu, unsigned char registerIndex, char *registerName) {
    fprintf (stderr, "%s: %lf", registerName, spu->registerValues [registerIndex]);
}

static void PrintImmed (elem_t value) {
    fprintf (stderr, "%lf", value);
}

static void PrintRegisterAndImmed (SPU *spu, unsigned char registerIndex, elem_t value) {
    fprintf (stderr, "%lf", value + spu->registerValues [registerIndex]);
}

static bool HasRamBrackets (TextLine *arguments) {
    PushLog (4);

    ssize_t end = FindActualStringEnd (arguments);

    if (arguments->pointer [0] == '[' && arguments->pointer [end] == ']') {
        RETURN true;
    }

    RETURN false;
}

static ProcessorErrorCode PlaceBreakpoint (SPU *spu, char *arguments, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer) {
    PushLog (3);

    DebugInfoChunk patternBreakpoint = {};
    DebugInfoChunk *foundAddress = NULL;

    arguments = strtok (arguments, " ");

    if (arguments [0] == '*' && sscanf (arguments + 1, "%lu", &patternBreakpoint.address) > 0) {
        foundAddress = FindValueInBuffer (debugInfoBuffer, &patternBreakpoint, DebugInfoChunkComparatorByAddress);

    } else if (sscanf (arguments, "%d", &patternBreakpoint.line) > 0) {
        foundAddress = FindValueInBuffer (debugInfoBuffer, &patternBreakpoint, DebugInfoChunkComparatorByLine);

    } else {
        PrintErrorMessage (WRONG_LINE, "Please enter valid line number or bytecode address", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN WRONG_LINE;
    }

    if (!foundAddress) {
        PrintErrorMessage (WRONG_LINE, "Please enter valid line number or bytecode address", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN WRONG_LINE;
    }

    WriteDataToBuffer (breakpointsBuffer, foundAddress, 1);

    RETURN NO_PROCESSOR_ERRORS;
}

static void ShowDebuggerLogo (FILE *stream) {
    PushLog (4);

    char logo [] = {
        "                               __     __\n"
        "  ____ __________ ___     ____/ /__  / /_  __  ______ _____ ____  _____\n"
        " / __ `/ ___/ __ `__ \\   / __  / _ \\/ __ \\/ / / / __ `/ __ `/ _ \\/ ___/\n"
        "/ /_/ (__  ) / / / / /  / /_/ /  __/ /_/ / /_/ / /_/ / /_/ /  __/ /\n"
        "\\__,_/____/_/ /_/ /_/   \\__,_/\\___/_.___/\\__,_/\\__, /\\__, /\\___/_/\n"
        "                                              /____//____/\n"
    };

    fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stream, "%s\n\nMaxim Timoshkin, MIPT, 2023\n\n\n",logo);

    RETURN;
}

long long DebugInfoChunkComparatorByLine (void *value1, void *value2) {
    PushLog (4);

    custom_assert (value1, pointer_is_null, -1);
    custom_assert (value2, pointer_is_null, -1);

    DebugInfoChunk *chunk1 = (DebugInfoChunk *) value1;
    DebugInfoChunk *chunk2 = (DebugInfoChunk *) value2;

    RETURN chunk1->line - chunk2->line;
}

long long DebugInfoChunkComparatorByAddress (void *value1, void *value2) {
        PushLog (4);

    custom_assert (value1, pointer_is_null, -1);
    custom_assert (value2, pointer_is_null, -1);

    DebugInfoChunk *chunk1 = (DebugInfoChunk *) value1;
    DebugInfoChunk *chunk2 = (DebugInfoChunk *) value2;

    RETURN (long long) (chunk1->address - chunk2->address);
}

