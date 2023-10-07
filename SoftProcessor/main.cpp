#include <stdio.h>
#include <sys/stat.h>

#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "ConsoleParser.h"
#include "Logger.h"
#include "SPU.h"
#include "SoftProcessor.h"
#include "TextTypes.h"
#include "Stack/Stack.h"

const size_t MAX_BINARY_FILES = 1024;

static char *BinaryFiles [MAX_BINARY_FILES] = {};
static size_t BinaryCount = 0;

void AddBinary (char **arguments);

int main (int argc, char **argv){
    PushLog (1);

    //Process console line arguments
    register_flag ("-b", "--binary", AddBinary, 1);
    parse_flags (argc, argv);

    //Read binary files
    FileBuffer fileBuffer = {};

    for (size_t fileIndex = 0; fileIndex < BinaryCount; fileIndex++) {
        if (!CreateFileBuffer (&fileBuffer, BinaryFiles [fileIndex])) {

            DestroyFileBuffer (&fileBuffer);
            continue;
        }

        if (!ReadFile (BinaryFiles [fileIndex], &fileBuffer)) {

            DestroyFileBuffer (&fileBuffer);
            continue;
        }

        SPU spu {
            .bytecode = &fileBuffer,
        };

        ExecuteFile (&spu);

        DestroyFileBuffer (&fileBuffer);
    }


    RETURN 0;
}

void AddBinary (char **arguments) {
    PushLog (3);

    if (IsRegularFile (arguments [0]) != 1){
        perror ("Error occuried while adding binary file");

        RETURN;
    }

    custom_assert (BinaryCount < MAX_BINARY_FILES, invalid_value, (void)0);

    if (BinaryCount < MAX_BINARY_FILES) {
        BinaryFiles [BinaryCount++] = arguments [0];
    }

    RETURN;
}
