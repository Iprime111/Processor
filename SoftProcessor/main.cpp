#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <cstddef>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "AssemblyHeader.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "ConsoleParser.h"
#include "MessageHandler.h"
#include "SPU.h"
#include "SoftProcessor.h"
#include "TextTypes.h"
#include "GraphicsProvider.h"
#include "Stack/Stack.h"

static char      *BinaryFile           = NULL;
static char      *SourceFile           = NULL;
static useconds_t FrequencyTime        = 0;
static bool       IsGraphicsEnabled    = false;

static sf::Mutex  WorkMutex            = {};

void AddBinary       (char **arguments);
void AddSource       (char **arguments);
void SetFrequency    (char **arguments);
void EnableDebugMode (char **arguments);
void EnableGraphics  (char **arguments);

static bool PrepareForExecuting (FileBuffer *fileBuffer);
void LaunchThread (SPU *spu);

int main (int argc, char **argv){
    PushLog (1);

    SetGlobalMessagePrefix ("Processor");
    SetDebugMode (false);

    //Process console line arguments
    register_flag ("-b", "--binary",    AddBinary,       1);
    register_flag ("-s", "--source",    AddSource,       1);
    register_flag ("-f", "--frequency", SetFrequency,    1);
    register_flag ("-d", "--debug",     EnableDebugMode, 0);
    register_flag ("-g", "--graphics",  EnableGraphics,  0);
    parse_flags   (argc, argv);

    //Read binary file
    FileBuffer fileBuffer = {};

    if (!PrepareForExecuting (&fileBuffer)) {
        RETURN 0;
    }

    SPU spu = {
        .bytecode        = fileBuffer,
        .frequencySleep  = FrequencyTime,
        .graphicsEnabled = IsGraphicsEnabled,
        .isWorking       = true,
    };

    if (IsGraphicsEnabled) {
        ProgramErrorCheck (InitCells (), "Error occuried while initializing ram graphics");
    }

    sf::Thread processorThread (&LaunchThread, &spu);
    processorThread.launch ();

    if (IsGraphicsEnabled) {
        sf::RenderWindow window (sf::VideoMode (WINDOW_X_SIZE, WINDOW_Y_SIZE), "RAM graphics", sf::Style::None);

        RenderLoop (&window, &spu, &WorkMutex);
    }

    processorThread.wait ();
    DestroyFileBuffer (&fileBuffer);

    RETURN 0;
}

void LaunchThread (SPU *spu) {
    LaunchProgram (spu, SourceFile, BinaryFile, &WorkMutex);
}

static bool PrepareForExecuting (FileBuffer *fileBuffer) {
    PushLog (2);

    if (!BinaryFile) {
        RETURN false;
    }

    if (!CreateFileBuffer (fileBuffer, BinaryFile)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while creating binary file buffer", NULL, NULL, -1);
        RETURN false;
    }

    if (!ReadFile (BinaryFile, fileBuffer)) {
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while reading binary", NULL, NULL, -1);
        RETURN false;
    }

    RETURN true;
}

void AddBinary (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    if (!IsRegularFile (arguments [0])){
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while adding binary file - not a regular file", NULL, NULL, -1);
        RETURN;
    }

    BinaryFile = arguments [0];

    RETURN;
}

void AddSource (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    if (!IsRegularFile (arguments [0])){
        PrintErrorMessage (INPUT_FILE_ERROR, "Error occuried while adding source file - not a regular file", NULL, NULL, -1);
        RETURN;
    }

    SourceFile = arguments [0];

    RETURN;
}


void SetFrequency (char **arguments) {
    PushLog (3);

    custom_assert (arguments,     pointer_is_null, (void)0);
    custom_assert (arguments [0], pointer_is_null, (void)0);

    unsigned int frequency = (unsigned int) atol (arguments [0]);

    if (frequency < MIN_FREQUENCY || frequency > MAX_FREQUENCY) {
        PrintWarningMessage (WRONG_FREQUENCY, "Bad frequency number. Setting frequency to maximum limit.", NULL, NULL, -1);

        frequency = MAX_FREQUENCY;
    }

    FrequencyTime = MAX_SLEEP_TIME - MAX_SLEEP_TIME * frequency / MAX_FREQUENCY;

    RETURN;
}

void EnableDebugMode (char **arguments) {
    PushLog (3);

    SetDebugMode (true);

    RETURN;
}

void EnableGraphics  (char **arguments) {
    PushLog (3);

    IsGraphicsEnabled = true;

    RETURN;
}
