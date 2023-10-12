#include "MessageHandler.h"
#include "CommonModules.h"
#include "ColorConsole.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "SecureStack/SecureStack.h"

#include <stdio.h>

static char *GlobalPrefix = NULL;

static CONSOLE_COLOR GetMessageColor (MessageType type);

static void PrintPrefix      (FILE *stream, const char *prefix,           CONSOLE_COLOR color);
static void PrintMessageText (FILE *stream, const char *text,             CONSOLE_COLOR color);
static void PrintError       (FILE *stream, ProcessorErrorCode errorCode, CONSOLE_COLOR color);

ProcessorErrorCode PrintMessage (FILE *stream, ProcessorMessage message) {
    PushLog (4);

    CONSOLE_COLOR messageColor = GetMessageColor (message.type);

    PrintPrefix      (stream, message.prefix,    messageColor);
    PrintMessageText (stream, message.text,      CONSOLE_WHITE);
    PrintError       (stream, message.errorCode, messageColor);

    fputs ("\n", stream);

    RETURN NO_PROCESSOR_ERRORS;
}

void SetGlobalMessagePrefix (char *newPrefix) {
    GlobalPrefix = newPrefix;
}

// TODO print error line
static void PrintError (FILE *stream, ProcessorErrorCode errorCode, CONSOLE_COLOR color) {
    PushLog (4);

    fprintf_color (color, CONSOLE_NORMAL, stream, "(");

    #define MSG_(errorCode, patternCode, message)                                                   \
                if (errorCode & patternCode) {                                                      \
                    fprintf_color (color, CONSOLE_NORMAL, stream, " " #patternCode ": " message);   \
                }

    if (errorCode == NO_PROCESSOR_ERRORS) {
        printf_color (color, CONSOLE_NORMAL, "No errors have been detected)");
        RETURN;
    }

    MSG_ (errorCode, WRONG_INSTRUCTION,  "Readed instruction do not exist");
    MSG_ (errorCode, BUFFER_ENDED,       "Command buffer ended");
    MSG_ (errorCode, NO_BUFFER,          "Command buffer do not exist");
    MSG_ (errorCode, STACK_ERROR,        "Error occuried in processor stack");
    MSG_ (errorCode, PROCESSOR_HALT,     "Halt command has been detected");
    MSG_ (errorCode, TOO_MANY_ARGUMENTS, "Given instruction requires less arguments to be passed");
    MSG_ (errorCode, TOO_FEW_ARGUMENTS,  "Given instruction requires more arguments to be passed");
    MSG_ (errorCode, OUTPUT_FILE_ERROR,  "Error occuried with output file");
    MSG_ (errorCode, INPUT_FILE_ERROR,   "Error occuried with input file");

    #undef MSG_

    fprintf_color (color, CONSOLE_NORMAL, stream, ") ");

    RETURN;
}


static void PrintMessageText (FILE *stream, const char *text, CONSOLE_COLOR color) {
    PushLog (4);

    if (text)
        fprintf_color (color, CONSOLE_NORMAL, stream, "%s ", text);

    RETURN;
}

static void PrintPrefix (FILE *stream, const char *prefix, CONSOLE_COLOR color) {
    PushLog (4);

    const char *currentPrefix = NULL;

    if (prefix) {
        currentPrefix = prefix;
    } else {
        if (GlobalPrefix)
            currentPrefix = GlobalPrefix;
    }

    if (currentPrefix) {
        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stream, "[");
        fprintf_color (color, CONSOLE_BOLD, stream, "%s", currentPrefix);
        fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stream, "]: ");
    }

    RETURN;
}

static CONSOLE_COLOR GetMessageColor (MessageType type) {
    PushLog (4);

    CONSOLE_COLOR messageColor = CONSOLE_DEFAULT;

    switch (type) {
        case INFO_MESSAGE:
            messageColor = CONSOLE_BLUE;
            break;

        case ERROR_MESSAGE:
            messageColor = CONSOLE_RED;
            break;

        case WARNING_MESSAGE:
            messageColor = CONSOLE_YELLOW;
            break;

        case SUCCESS_MESSAGE:
            messageColor = CONSOLE_GREEN;
            break;

        default:
            messageColor = CONSOLE_DEFAULT;
            break;
    }

    RETURN messageColor;
}
