#include <cstdio>
#include <stdio.h>
#include <string.h>

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

static ProcessorErrorCode ShowCustomPromptLine ();
static ProcessorErrorCode PlaceBreakpoint  (SPU *spu, char *arguments, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer);
static ProcessorErrorCode PrintSpuData     (SPU *spu, char *arguments);

static void PrintMemoryValue      (SPU *spu, ssize_t address, char *arguments);
static void PrintRegister         (SPU *spu, unsigned char registerIndex, char *registerName);
static void PrintRegisterAndImmed (SPU *spu, unsigned char registerIndex, elem_t value);
static void PrintImmed            (elem_t value);

static bool HasRamBrackets (TextLine *arguments);
static void ShowDebuggerLogo (FILE *stream);

static char DEBUGGER_ERROR_PREFIX [] = "Debugger";

ProcessorErrorCode InitDebugConsole () {
    PushLog (3);

    fprintf (stderr, CLEAR_SCREEN());
    ShowDebuggerLogo (stderr);

    RETURN NO_PROCESSOR_ERRORS;
}

DebuggerAction BreakpointStop (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, const DebugInfoChunk *breakpointData, TextBuffer *text) {
    PushLog (2);

    fprintf (stderr, "Break: " BOLD_WHITE_COLOR "%s\n", text->lines [breakpointData->line - 1].pointer);

    RETURN DebugConsole (spu, debugInfoBuffer, breakpointsBuffer);
}

DebuggerAction DebugConsole (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer) {
    PushLog (2);

    Buffer <char> commandBuffer = {0, 0, NULL};

    #define DestroyBufferAndReturn(returnValue) \
            DestroyBuffer (&commandBuffer);     \
            RETURN returnValue;                 \

    while (true) {
        ShowCustomPromptLine ();
        getline (&commandBuffer.data, &commandBuffer.capacity, stdin);

        char commandName [MAX_DEBUGGER_COMMAND_LENGTH] = "";
        int commandNameLength = 0;
        sscanf (commandBuffer.data, "%s%n", commandName, &commandNameLength);

        char *argumentsLine = commandBuffer.data + commandNameLength;

        #define DEBUGGER_COMMAND_(name, shortName, ...)                               \
            if (!strcmp (commandName, name) || !strcmp (commandName, shortName)) {    \
                __VA_ARGS__;                                                          \
            }                                                                         \

        //TODO error detection
        DEBUGGER_COMMAND_ ("quit",       "q", DestroyBufferAndReturn (QUIT_PROGRAM))
        DEBUGGER_COMMAND_ ("continue",   "c", DestroyBufferAndReturn (CONTINUE_PROGRAM));
        DEBUGGER_COMMAND_ ("run",        "r", DestroyBufferAndReturn (RUN_PROGRAM));
        DEBUGGER_COMMAND_ ("print",      "p", PrintSpuData    (spu, argumentsLine));
        DEBUGGER_COMMAND_ ("breakpoint", "b", PlaceBreakpoint (spu, argumentsLine, debugInfoBuffer, breakpointsBuffer));

        #undef DEBUGGER_COMMAND_
    }

    DestroyBufferAndReturn (CONTINUE_PROGRAM);

    #undef DestroyBufferAndReturn
}

ProcessorErrorCode ReadSourceFile (FileBuffer *fileBuffer, TextBuffer *text, char *filename) {
    PushLog (3);

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


static ProcessorErrorCode PrintSpuData (SPU *spu, char *arguments) {
    PushLog (3);

    arguments = strtok (arguments, " ");
    TextLine argumentsLine = {arguments, strlen (arguments)};

    bool isRamValue = HasRamBrackets (&argumentsLine);
    if (isRamValue) {
        argumentsLine.length--;
        argumentsLine.pointer [argumentsLine.length - 1] = '\0';
        argumentsLine.pointer++;
    }

    #define FIND_REGISTER()                                                         \
            const Register *foundRegister = FindRegisterByName (registerName);      \
            if (foundRegister) {                                                    \
                registerIndex = foundRegister->index;                               \
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
        if (!isRamValue)
            PrintRegister (spu, registerIndex, registerName);
        else
            value = spu->registerValues [registerIndex];

    } else {
        PrintErrorMessage (TOO_FEW_ARGUMENTS, "Wrong arguments prompt", DEBUGGER_ERROR_PREFIX, NULL, -1);
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

    int lineNumber = -1;
    if (sscanf (arguments, "%d", &lineNumber) <= 0) {
        PrintErrorMessage (WRONG_LINE, "Please enter valid line number", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN WRONG_LINE;
    }

    DebugInfoChunk lineByNumber = {0, lineNumber};
    const DebugInfoChunk *foundAddress = FindValueInBuffer (debugInfoBuffer, &lineByNumber, DebugInfoChunkComparatorByLine);
    if (!foundAddress) {
        PrintErrorMessage (WRONG_LINE, "Please enter valid line number", DEBUGGER_ERROR_PREFIX, NULL, -1);
        RETURN WRONG_LINE;
    }

    WriteDataToBuffer (breakpointsBuffer, foundAddress, 1);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ShowCustomPromptLine () {
    PushLog (4);

    fprintf(stderr, WHITE_COLOR "Assembler debugger" BOLD_WHITE_COLOR " > ");

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

