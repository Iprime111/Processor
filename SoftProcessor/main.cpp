#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "ConsoleParser.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"
#include "SoftProcessor.h"
#include "TextTypes.h"
#include "Stack/Stack.h"

#include <cstddef>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

static char      *BinaryFile    = NULL;
static useconds_t FrequencyTime = 0;

void AddBinary (char **arguments);
void SetFrequency (char **arguments);

static bool PrepareForExecuting (FileBuffer *fileBuffer);

int main (int argc, char **argv){
    PushLog (1);

    SetGlobalMessagePrefix ("Processor");

    //Process console line arguments
    register_flag ("-b", "--binary",    AddBinary,    1);
    register_flag ("-f", "--frequency", SetFrequency, 1);
    parse_flags (argc, argv);

    //Read binary file
    FileBuffer fileBuffer = {};

    if (PrepareForExecuting (&fileBuffer)){
        SPU spu {
            .bytecode = &fileBuffer,
            .frequencySleep = FrequencyTime,
        };

        ExecuteFile (&spu);
    }

    DestroyFileBuffer (&fileBuffer);

    RETURN 0;
}

static bool PrepareForExecuting (FileBuffer *fileBuffer) {
    PushLog (2);

    if (!BinaryFile) {
        RETURN false;
    }

    if (!CreateFileBuffer (fileBuffer, BinaryFile)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while creating binary file buffer", NULL, -1);
        RETURN false;
    }

    if (!ReadFile (BinaryFile, fileBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while reading binary", NULL, -1);
        RETURN false;
    }

    RETURN true;
}

void AddBinary (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    if (!IsRegularFile (arguments [0])){
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while adding binary file - not a regular file", NULL, -1);
        RETURN;
    }

    BinaryFile = arguments [0];

    RETURN;
}

void SetFrequency (char **arguments) {
    PushLog (3);

    unsigned int frequency = (unsigned int) atol (arguments [0]);

    if (frequency < MIN_FREQUENCY || frequency > MAX_FREQUENCY) {
        PrintWarningMessage (WRONG_FREQUENCY, "Bad frequency number. Setting frequency to maximum limit.", NULL, -1);

        frequency = MAX_FREQUENCY;
    }

    FrequencyTime = MAX_SLEEP_TIME - MAX_SLEEP_TIME * (frequency / MAX_FREQUENCY);

    RETURN;
}
