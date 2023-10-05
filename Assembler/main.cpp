#include <stdlib.h>

#include "ConsoleParser.h"
#include "FileIO.h"
#include "Logger.h"
#include "TextTypes.h"
#include "Assembler.h"

const size_t MAX_SOURCE_FILES = 1024;

static char *SourceFiles [MAX_SOURCE_FILES] = {};
static size_t SourceCount = 0;

void AddSource (char **arguments);

int main (int argc, char **argv) {
    PushLog (1);

    //Process console line arguments
    register_flag ("-s", "--source", AddSource, 1);
    parse_flags (argc, argv);

    //Process source files
    FileBuffer fileBuffer = {};
    TextBuffer textBuffer = {};

    for (size_t fileIndex = 0; fileIndex < SourceCount; fileIndex++) {
        bool shouldAssemble = true;

        if (!CreateFileBuffer (&fileBuffer, SourceFiles [fileIndex]))
            shouldAssemble = false;

        if (!ReadFileLines (SourceFiles [fileIndex], &fileBuffer, &textBuffer))
            shouldAssemble = false;

        int outFileDescriptor = -1;
        if ((outFileDescriptor = OpenFileWrite ("hui")) == -1)
            shouldAssemble = false;

        if (shouldAssemble) {
            AssembleFile (&textBuffer, outFileDescriptor);

            CloseFile (outFileDescriptor);
        }

        DestroyFileBuffer (&fileBuffer);
        free (textBuffer.lines);
    }

    RETURN 0;
}

void AddSource (char **arguments) {
    PushLog (3);

    if (IsRegularFile (arguments [0]) != 1){
        perror ("Error occuried while adding source file file");

        RETURN;
    }

    custom_assert (SourceCount < MAX_SOURCE_FILES, invalid_value, (void)0);

    if (SourceCount < MAX_SOURCE_FILES) {
        SourceFiles [SourceCount++] = arguments [0];
    }

    RETURN;
}

