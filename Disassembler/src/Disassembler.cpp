#include "Disassembler.h"
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
#include <cstring>

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu);

ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    spu->ip = 0;
    CheckBuffer (spu);

    while (ReadInstruction(outFileDescriptor, spu) == NO_PROCESSOR_ERRORS);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    ON_DEBUG (printf ("Reading command: "));

    CommandCode commandCode {0, 0};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

    if (!instruction){
        PrintErrorMessage (WRONG_INSTRUCTION, "Wrong instruction readed", NULL);
        RETURN WRONG_INSTRUCTION;
    }

    ON_DEBUG (printf ("%s\n", instruction->instructionName));

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    strcat (commandLine, instruction->instructionName);

    char registerIndex = -1;
    elem_t immedArgument = 0;
    char *registerName = NULL;

    #define REGISTER(NAME, INDEX)               \
                if (registerIndex == INDEX) {   \
                    registerName = #NAME;       \
                }

    if (commandCode.arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT)) {
        ReadData (spu, &registerIndex, char);
        ReadData (spu, &immedArgument, elem_t);

        #include "Registers.def"

        if (!registerName) {
            PrintErrorMessage (TOO_FEW_ARGUMENTS, "Wrong register name format", NULL);
            RETURN TOO_FEW_ARGUMENTS;
        }

        sprintf (commandLine, "%s %s+%lf\n", instruction->instructionName, registerName, immedArgument);

    }else if (commandCode.arguments & IMMED_ARGUMENT) {
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, "%s %lf\n", instruction->instructionName, immedArgument);
    }else if (commandCode.arguments & REGISTER_ARGUMENT){
        ReadData (spu, &registerIndex, char);

        #include "Registers.def"

        if (!registerName) {
            PrintErrorMessage (TOO_FEW_ARGUMENTS, "Wrong register name format", NULL);
            RETURN TOO_FEW_ARGUMENTS;
        }

        sprintf (commandLine, "%s %s\n", instruction->instructionName, registerName);

    }else {
        sprintf (commandLine, "%s\n", instruction->instructionName);
    }

    #undef REGISTER

    if (!WriteBuffer (outFileDescriptor, commandLine, (ssize_t) strlen (commandLine))) {
        PrintErrorMessage (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file", NULL);
        RETURN OUTPUT_FILE_ERROR;
    }


    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK) \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                              \
                return NO_PROCESSOR_ERRORS;                                     \
            }

#include "Instructions.def"

#undef INSTRUCTION
