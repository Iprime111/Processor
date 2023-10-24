#include <stdio.h>

#include "MessageHandler.h"
#include "CommonModules.h"
#include "ColorConsole.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "SecureStack/SecureStack.h"

static char *GlobalPrefix = NULL;

static CONSOLE_COLOR GetMessageColor (MessageType type);

static void PrintPrefix        (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color);
static void PrintMessageText   (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color);
static void PrintErrorLocation (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color);
static void PrintErrorCode     (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color);
static void PrintAsmErrorLine  (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color);

ProcessorErrorCode PrintMessage (FILE *stream, ProcessorMessage message) {
    PushLog (4);

    CONSOLE_COLOR messageColor = GetMessageColor (message.type);

    PrintPrefix      (stream, &message, messageColor);
    PrintMessageText (stream, &message, CONSOLE_WHITE);

    if (message.errorCode == NO_PROCESSOR_ERRORS) {
        fprintf (stream, "\n");
        RETURN NO_PROCESSOR_ERRORS;
    }

    ON_DEBUG (PrintErrorLocation (stream, &message, messageColor));
    fprintf (stream, "\n");

    PrintErrorCode (stream, &message, messageColor);

    if (message.asmLine && message.asmLineNumber >= 0) {
        PrintAsmErrorLine (stream, &message, messageColor);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

void SetGlobalMessagePrefix (char *newPrefix) {
    GlobalPrefix = newPrefix;
}

static void PrintAsmErrorLine  (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color) {
    PushLog (4);

    custom_assert (stream,           pointer_is_null, (void)0);
    custom_assert (message,          pointer_is_null, (void)0);
    custom_assert (message->asmLine, pointer_is_null, (void)0);

    fprintf_color (CONSOLE_WHITE, CONSOLE_NORMAL, stream, MOVE_CURSOR_FORWARD (6)"|\n");

    fprintf_color (CONSOLE_WHITE, CONSOLE_NORMAL, stream, "%5d |", message->asmLineNumber);
    fprintf_color (CONSOLE_BLUE,  CONSOLE_BOLD,   stream, " %s\n", message->asmLine->pointer);

    fprintf_color (CONSOLE_WHITE, CONSOLE_NORMAL, stream, MOVE_CURSOR_FORWARD (6)"|\n");

    RETURN;
}

static void PrintErrorCode (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color) {
    PushLog (4);

    custom_assert (stream,  pointer_is_null, (void)0);
    custom_assert (message, pointer_is_null, (void)0);

    #define MSG_(errorCode, patternCode, message)                                                                   \
                if (errorCode & patternCode) {                                                                      \
                    fprintf_color (color, CONSOLE_NORMAL, stream, " " #patternCode ": " WHITE_COLOR message "\n");  \
                }

    MSG_ (message->errorCode, WRONG_INSTRUCTION,  "Readed instruction do not exist");
    MSG_ (message->errorCode, BUFFER_ENDED,       "Command buffer ended");
    MSG_ (message->errorCode, NO_BUFFER,          "Command buffer do not exist");
    MSG_ (message->errorCode, STACK_ERROR,        "Error occuried in processor stack");
    MSG_ (message->errorCode, PROCESSOR_HALT,     "Halt command has been detected");
    MSG_ (message->errorCode, TOO_MANY_ARGUMENTS, "Given instruction requires less arguments to be passed");
    MSG_ (message->errorCode, TOO_FEW_ARGUMENTS,  "Given instruction requires more arguments to be passed");
    MSG_ (message->errorCode, OUTPUT_FILE_ERROR,  "Error occuried with output file");
    MSG_ (message->errorCode, INPUT_FILE_ERROR,   "Error occuried with input file");
    MSG_ (message->errorCode, BLANK_LINE,         "Blank line has been found");
    MSG_ (message->errorCode, NO_PROCESSOR,       "No available processor has been found");
    MSG_ (message->errorCode, WRONG_HEADER,       "Header has wrong format");
    MSG_ (message->errorCode, WRONG_LABEL,        "Label do not exist");
    MSG_ (message->errorCode, WRONG_FREQUENCY,    "Frequency is out of bounds");

    #undef MSG_


    RETURN;
}

static void PrintErrorLocation (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color) {
    PushLog (4);

    custom_assert (stream,  pointer_is_null, (void)0);
    custom_assert (message, pointer_is_null, (void)0);

    fprintf_color (color, CONSOLE_NORMAL, stream, "[%s:%d in %s]", message->file, message->line, message->function);
    RETURN;
}

static void PrintMessageText (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color) {
    PushLog (4);

    custom_assert (stream,  pointer_is_null, (void)0);
    custom_assert (message, pointer_is_null, (void)0);

    if (message->text)
        fprintf_color (color, CONSOLE_NORMAL, stream, "%s ", message->text);

    RETURN;
}

static void PrintPrefix (FILE *stream, ProcessorMessage *message, CONSOLE_COLOR color) {
    PushLog (4);

    custom_assert (stream,  pointer_is_null, (void)0);
    custom_assert (message, pointer_is_null, (void)0);

    const char *currentPrefix = "No prefix";

    if (message->prefix) {
        currentPrefix = message->prefix;
    } else if (GlobalPrefix){
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

