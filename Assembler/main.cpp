#include <bits/types/FILE.h>
#include <cstdio>
#include <stdlib.h>// TODO

#include "ConsoleParser.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SecureStack/SecureStack.h"
#include "TextTypes.h"
#include "Assembler.h"

static char *SourceFile = NULL;
static char *BinaryFile = "a.out"; // TODO think 'bout it

void AddSource (char **arguments);
void AddBinary (char **arguments);

static bool PrepareForAssembling (FileBuffer *fileBuffer, TextBuffer *textBuffer, int *outFileDescriptor);

// TODO create the best listing
int main (int argc, char **argv) {
    PushLog (1);

    SetGlobalMessagePrefix ("Assembler");

    //Process console line arguments
    register_flag ("-s", "--source", AddSource, 1);
    register_flag ("-o", "--output", AddBinary, 1);
    parse_flags (argc, argv);
    // TODO parser -> stores flags -> get("...")
    // TODO: get_flag(parser, "binary")

    //Process source files
    FileBuffer fileBuffer = {};
    TextBuffer textBuffer = {};

    int outFileDescriptor = -1;

    if (PrepareForAssembling (&fileBuffer, &textBuffer, &outFileDescriptor)) {
        AssembleFile (&textBuffer, outFileDescriptor);
        CloseFile (outFileDescriptor);
    }

    DestroyFileBuffer (&fileBuffer);
    free (textBuffer.lines);

    RETURN 0;
}

void AddSource (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    if (IsRegularFile (arguments [0])) {
        SourceFile = arguments [0];
    }
}

void AddBinary (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    BinaryFile = arguments [0];

    RETURN;
}

static bool PrepareForAssembling (FileBuffer *fileBuffer, TextBuffer *textBuffer, int *outFileDescriptor) {
    PushLog (2);

    if (!SourceFile) {
        RETURN false;
    }

    if (!CreateFileBuffer (fileBuffer, SourceFile)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while creating file buffer", NULL);
        RETURN false;
    }

    if (!ReadFileLines (SourceFile, fileBuffer, textBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while reading file lines", NULL);
        RETURN false;
    }

    if (!ChangeNewLinesToZeroes (textBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while changing new line symbols to zero symbols", NULL);
        RETURN false;
    }

    if ((*outFileDescriptor = OpenFileWrite (BinaryFile)) == -1) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while opening binary file", NULL);
        RETURN false;
    }

    RETURN true;
}

