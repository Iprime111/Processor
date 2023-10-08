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

ProcessorErrorCode PrintMessage (FILE *stream, ProcessorMessage message);
void SetGlobalMessagePrefix (char *newPrefix);

#define PrintErrorMessage(errorCode, errorMessage, messagePrefix)   PrintMessage (stderr, {errorCode, ERROR_MESSAGE,   errorMessage, messagePrefix, 0})
#define PrintWarningMessage(errorCode, errorMessage, messagePrefix) PrintMessage (stderr, {errorCode, WARNING_MESSAGE, errorMessage, messagePrefix, 0})
#define PrintInfoMessage(errorCode, errorMessage, messagePrefix)    PrintMessage (stdout, {errorCode, INFO_MESSAGE,    errorMessage, messagePrefix, 0})
#define PrintSuccessMessage(errorCode, errorMessage, messagePrefix) PrintMessage (stdout, {errorCode, SUCCESS_MESSAGE, errorMessage, messagePrefix, 0})

#endif
