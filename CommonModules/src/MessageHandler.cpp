#include "MessageHandler.h"
#include "CommonModules.h"
#include "ColorConsole.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "SecureStack/SecureStack.h"
#include <cstdio>

// TODO print error (or warging) code
static CONSOLE_COLOR GetMessageColor (MessageType type);

static void PrintPrefix      (const char *prefix, CONSOLE_COLOR color);
static void PrintMessageText (const char *text,   CONSOLE_COLOR color);

ProcessorErrorCode PrintMessage (ProcessorMessage *message) {
    PushLog (4);
    custom_assert (message, pointer_is_null, TOO_FEW_ARGUMENTS);

    CONSOLE_COLOR messageColor = GetMessageColor (message->type);

    if (message->prefix)
        PrintPrefix (message->prefix, messageColor);

    if (message->text)
        PrintMessageText (message->text, messageColor);


    RETURN NO_PROCESSOR_ERRORS;
}

static void PrintMessageText (const char *text, CONSOLE_COLOR color) {
    fprintf_color (color, CONSOLE_NORMAL, stderr, "%s\n", text);
}

static void PrintPrefix (const char *prefix, CONSOLE_COLOR color) {
    PushLog (4);
    custom_assert (prefix, pointer_is_null, (void)0);

    fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "[");
    fprintf_color (color, CONSOLE_BOLD, stderr, "%s", prefix);
    fprintf_color (CONSOLE_WHITE, CONSOLE_BOLD, stderr, "]: ");

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
