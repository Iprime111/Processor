#include <ctype.h>
#include <string.h>

#include "StringProcessing.h"
#include "CustomAssert.h"

ssize_t CountWhitespaces (TextLine *line) {
    PushLog (3);

    custom_assert (line,          pointer_is_null, -1);
    custom_assert (line->pointer, pointer_is_null, -1);

    ssize_t lineBegin = FindActualStringBegin (line);
    ssize_t lineEnd   = FindActualStringEnd   (line);

    if (lineBegin < 0 || lineEnd < 0) {
        RETURN -1;
    }

    ssize_t spaceCount = 0;

    for (ssize_t charIndex = lineBegin; charIndex < lineEnd; charIndex++) {
        if (isspace (line->pointer [charIndex]) || line->pointer [charIndex] == '+')
            spaceCount++;
    }

    RETURN spaceCount;
}

#define DetectWhitespacePosition(FOR_PREDICATE)                 \
            custom_assert (line,          pointer_is_null, -1); \
            custom_assert (line->pointer, pointer_is_null, -1); \
            for (FOR_PREDICATE) {                               \
                if (!isspace (line->pointer [charPointer])){    \
                    RETURN (ssize_t) charPointer;               \
                }                                               \
            }                                                   \
            RETURN -1

ssize_t FindActualStringEnd (TextLine *line) {
    PushLog (3);

    size_t length = line->length - 1;

    char *splitterPointer = strchr (line->pointer, ';');
    if (splitterPointer) {
        length = (size_t) (splitterPointer - line->pointer - 1);
    }

    DetectWhitespacePosition (ssize_t charPointer = (ssize_t) length; charPointer >= 0; charPointer--);
}

ssize_t FindActualStringBegin (TextLine *line) {
    PushLog (3);

    DetectWhitespacePosition (size_t charPointer = 0; charPointer < line->length; charPointer++);
}

#undef DetectWhitespacePosition

bool IsLabelLine (TextLine *line, char *labelName) {
    PushLog (4);

    custom_assert (line,            pointer_is_null, false);
    custom_assert (line->pointer,   pointer_is_null, false);

    RETURN line->pointer [FindActualStringEnd (line)] == ':' && sscanf (line->pointer, " %s", labelName) > 0;
}

ProcessorErrorCode DeleteExcessWhitespaces (TextBuffer *lines) {
    PushLog (3);

    custom_assert (lines,        pointer_is_null, NO_BUFFER);
    custom_assert (lines->lines, pointer_is_null, NO_BUFFER);

    for (size_t lineIndex = 0; lineIndex < lines->line_count; lineIndex++) {
        TextLine *currentLine = lines->lines + lineIndex;

        bool metWhitespace = false;
        char *writingPointer = currentLine->pointer;

        for (size_t symbolIndex = 0; symbolIndex < currentLine->length; symbolIndex++) {
            if (isspace (currentLine->pointer [symbolIndex])) {
                if (!metWhitespace) {
                    metWhitespace = true;
                } else {
                    continue;
                }

            } else {
                metWhitespace = false;
            }

            *writingPointer = currentLine->pointer [symbolIndex];
            writingPointer++;
        }

        *writingPointer = '\0';
        currentLine->length = (size_t) (writingPointer - currentLine->pointer);
    }

    RETURN NO_PROCESSOR_ERRORS;
}
