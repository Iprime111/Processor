#ifndef COMPILATION_ERROR_HANDLER_H_
#define COMPILATION_ERROR_HANDLER_H_

#include "CommonModules.h"
#include "SecureStack/SecureStack.h"

struct ProcessorMessage {
    ProcessorErrorCode errorCode;
    MessageType type;
    const char *text;
    const char *prefix;

    const int line;
    const char *file;
    const char *function;

    const int asmLine;
};

ProcessorErrorCode PrintMessage (FILE *stream, ProcessorMessage message);
void SetGlobalMessagePrefix (char *newPrefix);

#define PrintErrorMessage(errorCode, errorMessage, messagePrefix, asmLine)   PrintMessage (stderr, {errorCode,           ERROR_MESSAGE,   errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, asmLine})
#define PrintWarningMessage(errorCode, errorMessage, messagePrefix, asmLine) PrintMessage (stderr, {errorCode,           WARNING_MESSAGE, errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, asmLine})
#define PrintInfoMessage(errorMessage, messagePrefix)                        PrintMessage (stdout, {NO_PROCESSOR_ERRORS, INFO_MESSAGE,    errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, 0})
#define PrintSuccessMessage(errorMessage, messagePrefix)                     PrintMessage (stdout, {NO_PROCESSOR_ERRORS, SUCCESS_MESSAGE, errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, 0})

#define ErrorFound(errorCode, errorMessage, asmLine)                            \
            do {                                                                \
                if (errorCode != NO_PROCESSOR_ERRORS) {                         \
                    PrintErrorMessage (errorCode, errorMessage, NULL, asmLine); \
                    RETURN errorCode;                                           \
                }                                                               \
            }while (0)

#define ErrorFoundInProgram(errorCode, errorMessage) ErrorFound (errorCode, errorMessage, -1)

#endif
