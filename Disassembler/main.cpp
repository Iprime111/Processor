#include <stdio.h>
#include <sys/stat.h>

#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "ConsoleParser.h"
#include "Logger.h"
#include "Disassembler.h"
#include "SPU.h"
#include "TextTypes.h"
#include "Stack/Stack.h"

static char *BinaryFile = NULL;
static char *OutFile    = "a.disasm";

void AddBinary  (char **arguments);
void AddOutFile (char **arguments);

static bool PrepareForDisassembling (FileBuffer *fileBuffer, int *outFileDescriptor);

int main (int argc, char **argv){
    PushLog (1);

    //Process console line arguments
    register_flag ("-b", "--binary", AddBinary, 1);
    register_flag ("-o", "--output", AddOutFile, 1);
    parse_flags (argc, argv);

    //Read binary files
    FileBuffer fileBuffer = {};
    int outFileDescriptor = -1;

    if (PrepareForDisassembling (&fileBuffer, &outFileDescriptor)) {
        SPU spu {
            .bytecode = &fileBuffer,
        };

        DisassembleFile (outFileDescriptor, &spu);

        CloseFile (outFileDescriptor);
    }

    DestroyFileBuffer (&fileBuffer);


    RETURN 0;
}

static bool PrepareForDisassembling (FileBuffer *fileBuffer, int *outFileDescriptor) {
    PushLog (2);

    if (!BinaryFile) {
        RETURN false;
    }

    if (!CreateFileBuffer (fileBuffer, BinaryFile)) {
        RETURN false;
    }

    if (!ReadFile (BinaryFile, fileBuffer)) {
        RETURN false;
    }

    if ((*outFileDescriptor = OpenFileWrite (OutFile)) == -1) {
        RETURN false;
    }

    RETURN true;
}

void AddBinary (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    if (!IsRegularFile (arguments [0])){
        perror ("Error occuried while adding binary file");

        RETURN;
    }

    BinaryFile = arguments [0];

    RETURN;
}


void AddOutFile (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    OutFile = arguments [0];

    RETURN;
}
