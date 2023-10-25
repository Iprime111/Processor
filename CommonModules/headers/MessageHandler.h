#ifndef COMPILATION_ERROR_HANDLER_H_
#define COMPILATION_ERROR_HANDLER_H_

#include "CommonModules.h"
#include "SecureStack/SecureStack.h"
#include "TextTypes.h"

const size_t MAX_MESSAGE_LENGTH = 256;

struct ProcessorMessage {
    const ProcessorErrorCode errorCode;
    const MessageType type;
    const char *text;
    const char *prefix;

    const int line;
    const char *file;
    const char *function;

    const TextLine *asmLine;
    const int asmLineNumber;
};

ProcessorErrorCode PrintMessage (FILE *stream, ProcessorMessage message);
void SetGlobalMessagePrefix (char *newPrefix);

#define PrintErrorMessage(errorCode, errorMessage, messagePrefix, asmLine, asmLineNumber) \
    PrintMessage (stderr, {errorCode,           ERROR_MESSAGE,   errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, asmLine, asmLineNumber})

#define PrintWarningMessage(errorCode, errorMessage, messagePrefix, asmLine, asmLineNumber)\
    PrintMessage (stderr, {errorCode,           WARNING_MESSAGE, errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, asmLine, asmLineNumber})

#define PrintInfoMessage(errorMessage, messagePrefix)\
    PrintMessage (stdout, {NO_PROCESSOR_ERRORS, INFO_MESSAGE,    errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, NULL, 0})

#define PrintSuccessMessage(errorMessage, messagePrefix)\
    PrintMessage (stdout, {NO_PROCESSOR_ERRORS, SUCCESS_MESSAGE, errorMessage, messagePrefix, __LINE__, __FILE__, __PRETTY_FUNCTION__, NULL, 0})

#define SyntaxErrorCheck(errorCode, errorMessage, asmLine, asmLineNumber)                       \
            do {                                                                                \
                if (errorCode != NO_PROCESSOR_ERRORS) {                                         \
                    PrintErrorMessage (errorCode, errorMessage, NULL, asmLine, asmLineNumber);  \
                    RETURN errorCode;                                                           \
                }                                                                               \
            }while (0)

#define ProgramErrorCheck(errorCode, errorMessage) SyntaxErrorCheck (errorCode, errorMessage, NULL, -1)

#endif
