#ifndef COMPILATION_ERROR_HANDLER_H_
#define COMPILATION_ERROR_HANDLER_H_

#include "CommonModules.h"
#include "SecureStack/SecureStack.h"

struct ProcessorMessage {
    ProcessorErrorCode errorCode;
    MessageType type;
    const char *text;
    const char *prefix;
    int line;
};

ProcessorErrorCode PrintMessage (ProcessorMessage *message);

#endif
