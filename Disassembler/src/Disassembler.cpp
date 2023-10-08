#include <cstdio>
#include <cstring>

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

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu);

// TODO delete disassembly file if process was aborted
ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    spu->currentChar = 0;
    CheckBuffer (spu);

    while (ReadInstruction(outFileDescriptor, spu) == NO_PROCESSOR_ERRORS);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    printf ("Reading command: ");

    CommandCode commandCode {};
    ReadData (spu, &commandCode, char);

    const AssemblerInstruction *instruction = FindInstructionByNumber (commandCode.opcode);

    if (!instruction){
        PrintErrorMessage (WRONG_INSTRUCTION, "Wrong instruction readed", NULL);
        RETURN WRONG_INSTRUCTION;
    }

    printf ("%s\n", instruction->instructionName);

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    strcat (commandLine, instruction->instructionName);

    char registerIndex = -1;
    elem_t immedArgument = 0;

    if (commandCode.hasImmedArgument && commandCode.hasRegisterArgument) {
        ReadData (spu, &registerIndex, char);
        ReadData (spu, &immedArgument, elem_t);

        if (registerIndex >= 0 && registerIndex < REGISTER_COUNT){
            sprintf (commandLine, "%s r%cx+%lf\n", instruction->instructionName, registerIndex + 'a', immedArgument);
        }else {
            PrintErrorMessage (TOO_FEW_ARGUMENTS, "Wrong register name format", NULL);
            RETURN TOO_FEW_ARGUMENTS;
        }
    }else if (commandCode.hasImmedArgument) {
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, "%s %lf\n", instruction->instructionName, immedArgument);
    }else if (commandCode.hasRegisterArgument){
        ReadData (spu, &registerIndex, char);

        if (registerIndex >= 0 && registerIndex < REGISTER_COUNT) {
            sprintf (commandLine, "%s r%cx\n", instruction->instructionName, registerIndex + 'a');
        }else {
            PrintErrorMessage (TOO_FEW_ARGUMENTS, "Wrong register name format", NULL);
            RETURN TOO_FEW_ARGUMENTS;
        }

    }else {
        sprintf (commandLine, "%s\n", instruction->instructionName);
    }

    if (!WriteBuffer (outFileDescriptor, commandLine, (ssize_t) strlen (commandLine))) {
        PrintErrorMessage (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file", NULL);
        RETURN OUTPUT_FILE_ERROR;
    }


    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)                       \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                              \
                return NO_PROCESSOR_ERRORS;                                                     \
            }

#include "Instructions.def"

#undef INSTRUCTION
