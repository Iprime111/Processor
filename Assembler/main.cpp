#include <bits/types/FILE.h>
#include <cstdio>
#include <stdlib.h>

#include "AssemblyHeader.h"
#include "CommonModules.h"
#include "ConsoleParser.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SecureStack/SecureStack.h"
#include "TextTypes.h"
#include "Assembler.h"

static char *SourceFile  = NULL;
static char *ListingFile = NULL;
static char *BinaryFile  = "a.out";

void AddSource       (char **arguments);
void AddBinary       (char **arguments);
void AddListing      (char **arguments);
void EnableDebugMode (char **arguments);

static bool PrepareForAssembling (FileBuffer *fileBuffer, TextBuffer *textBuffer, int *binaryDescriptor, int *listingDescriptor);

int main (int argc, char **argv) {
    PushLog (1);

    SetGlobalMessagePrefix ("Assembler");
    SetDebugMode (false);

    //Process console line arguments
    register_flag ("-s", "--source",  AddSource,       1);
    register_flag ("-l", "--listing", AddListing,      1);
    register_flag ("-o", "--output",  AddBinary,       1);
    register_flag ("-d", "--debug",   EnableDebugMode, 0);
    parse_flags (argc, argv);

    //Process source files
    FileBuffer fileBuffer = {};
    TextBuffer textBuffer = {};

    int binaryDescriptor  = -1;
    int listingDescriptor = -1;

    if (PrepareForAssembling (&fileBuffer, &textBuffer, &binaryDescriptor, &listingDescriptor)) {
        ProcessorErrorCode errorCode = AssembleFile (&textBuffer, &fileBuffer, binaryDescriptor, listingDescriptor);

        CloseFile (binaryDescriptor);
        if (listingDescriptor != -1)
            CloseFile (listingDescriptor);

        if (errorCode != NO_PROCESSOR_ERRORS) {
            if (remove (BinaryFile)) {
                PrintErrorMessage (OUTPUT_FILE_ERROR, "Unable to delete corrupted binary file", NULL, NULL, -1);
            }
        }
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

    RETURN;
}

void AddBinary (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    BinaryFile = arguments [0];

    RETURN;
}

void AddListing (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    ListingFile = arguments [0];

    RETURN;
}

static bool PrepareForAssembling (FileBuffer *fileBuffer, TextBuffer *textBuffer, int *binaryDescriptor, int *listingDescriptor) {
    PushLog (2);

    if (!SourceFile) {
        RETURN false;
    }

    if (!CreateFileBuffer (fileBuffer, SourceFile)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while creating file buffer", NULL, NULL, -1);
        RETURN false;
    }

    if (!ReadFileLines (SourceFile, fileBuffer, textBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while reading file lines", NULL, NULL, -1);
        RETURN false;
    }

    if (!ChangeNewLinesToZeroes (textBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while changing new line symbols to zero symbols", NULL, NULL, -1);
        RETURN false;
    }

    if ((*binaryDescriptor = OpenFileWrite (BinaryFile)) == -1) {
            PrintErrorMessage (OUTPUT_FILE_ERROR, "Error occuried while opening binary file", NULL, NULL, -1);
            RETURN false;
        }

    if (ListingFile) {
        if ((*listingDescriptor = OpenFileWrite (ListingFile)) == -1) {
            PrintErrorMessage (OUTPUT_FILE_ERROR, "Error occuried while opening listing file", NULL, NULL, -1);
            RETURN false;
        }
    }else {
        *listingDescriptor = -1;
        PrintWarningMessage (OUTPUT_FILE_ERROR, "Lising file is not specified", NULL, NULL, -1);
    }


    RETURN true;
}

void EnableDebugMode (char **arguments) {
    PushLog (3);

    SetDebugMode (true);

    RETURN;
}


