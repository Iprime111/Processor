#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stddef.h>

#include "Buffer.h"
#include "CommonModules.h"
#include "SPU.h"
#include "TextTypes.h"

const size_t MAX_DEBUGGER_COMMAND_LENGTH         = 64;
const size_t DEFAULT_BREAKPOINTS_BUFFER_CAPACITY = 64;

enum DebuggerAction {
    RUN_PROGRAM             = 1,
    QUIT_PROGRAM            = 2,
    CONTINUE_PROGRAM        = 3,
};

ProcessorErrorCode InitDebugConsole ();
ProcessorErrorCode ReadSourceFile   (FileBuffer *file, TextBuffer *text, char *filename);

DebuggerAction DebugConsole   (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer);
DebuggerAction BreakpointStop (SPU *spu, Buffer <DebugInfoChunk> *debugInfoBuffer, Buffer <DebugInfoChunk> *breakpointsBuffer, const DebugInfoChunk *breakpointData, TextBuffer *text);

long long DebugInfoChunkComparatorByLine    (void *value1, void *value2);
long long DebugInfoChunkComparatorByAddress (void *value1, void *value2);

#endif
