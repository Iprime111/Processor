#include "Disassembler.h"
#include "Buffer.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static ProcessorErrorCode ReadInstruction (Buffer *disassemblyBuffer, SPU *spu);
static ProcessorErrorCode ReadArguments (const AssemblerInstruction *instruction, CommandCode *commandCode, SPU *spu, char *commandLine);
static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer *disassemblyBuffer);

static ProcessorErrorCode CreateDisassemblyBuffer (Buffer *disassemblyBuffer, SPU *spu);
static ProcessorErrorCode DestroyDisassemblyBuffer (Buffer *disassemblyBuffer);

ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    CheckBuffer (spu);

    Buffer disassemblyBuffer {0, 0, NULL};
    CreateDisassemblyBuffer(&disassemblyBuffer, spu);

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    while ((errorCode = ReadInstruction(&disassemblyBuffer, spu)) == NO_PROCESSOR_ERRORS);

    if (errorCode != BUFFER_ENDED) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffer (&disassemblyBuffer) | errorCode);
        ErrorFound (errorCode, "Disassembly error occuried");
    }

    if ((errorCode = WriteDisassemblyData (outFileDescriptor, &disassemblyBuffer)) != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffer (&disassemblyBuffer) | errorCode);
        RETURN errorCode;
    }

    RETURN DestroyDisassemblyBuffer (&disassemblyBuffer);
}

static ProcessorErrorCode CreateDisassemblyBuffer (Buffer *disassemblyBuffer, SPU *spu) {
    PushLog (3);

    //Disassembly format:
    // ip     line
    //0000\t  push ...
    //worst case: 1 byte of instruction = 9 bytes of symbols + '\n' symbol = 10 bytes

    disassemblyBuffer->capacity = (size_t) spu->bytecode->buffer_size * 10;

    disassemblyBuffer->data = (char *) calloc (disassemblyBuffer->capacity, sizeof (char));

    if (!disassemblyBuffer->data) {
        ErrorFound (NO_BUFFER, "Unable to create disassembly file buffer");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyDisassemblyBuffer (Buffer *disassemblyBuffer) {
    PushLog (3);

    if (disassemblyBuffer) {
        free (disassemblyBuffer->data);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer *disassemblyBuffer) {
    PushLog (2);

    if (!WriteBuffer (outFileDescriptor, disassemblyBuffer->data, (ssize_t) disassemblyBuffer->currentIndex)) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (Buffer *disassemblyBuffer, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    ON_DEBUG (printf ("Reading command: "));

    CommandCode commandCode {0, 0};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

    if (!instruction){
        ErrorFound (WRONG_INSTRUCTION, "Wrong instruction readed");
    }

    ON_DEBUG (printf ("%s\n", instruction->instructionName));

    if ((~instruction->commandCode.arguments) & commandCode.arguments) {
        ErrorFound (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
    }

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    int bytesPrinted = 0;
    sprintf (commandLine, "%.4lu\t%s%n", spu->ip - sizeof (commandCode), instruction->instructionName, &bytesPrinted);

    ProcessorErrorCode errorCode = ReadArguments (instruction, &commandCode, spu, commandLine + bytesPrinted);
    if (errorCode != NO_PROCESSOR_ERRORS) {
        ErrorFound (errorCode, "Error occuried while reading function arguments");
    }

    RETURN WriteDataToBuffer (disassemblyBuffer, commandLine, strlen (commandLine));
}

static ProcessorErrorCode ReadArguments (const AssemblerInstruction *instruction, CommandCode *commandCode, SPU *spu, char *commandLine) {
    PushLog (3);

    char registerIndex = -1;
    elem_t immedArgument = 0;
    char *registerName = NULL;

    #define REGISTER(NAME, INDEX)               \
                if (registerIndex == INDEX) {   \
                    registerName = #NAME;       \
                }

    if (commandCode->arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT)) {
        ReadData (spu, &registerIndex, char);
        ReadData (spu, &immedArgument, elem_t);

        #include "Registers.def"

        if (!registerName) {
            ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, " %s+%lf\n", registerName, immedArgument);

    }else if (commandCode->arguments & IMMED_ARGUMENT) {
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, " %lf\n", immedArgument);
    }else if (commandCode->arguments & REGISTER_ARGUMENT){
        ReadData (spu, &registerIndex, char);

        #include "Registers.def"

        if (!registerName) {
            ErrorFound (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, " %s\n", registerName);
    }else {
        sprintf (commandLine, "\n");
    }

    #undef REGISTER

    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK) \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                              \
                return NO_PROCESSOR_ERRORS;                                     \
            }

#include "Instructions.def"

#undef INSTRUCTION
