#include <cstdio>
#include <cstring>

#include "Disassembler.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu);

ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    spu->currentChar = 0;
    CheckBuffer (spu);

    StackInitDefault_ (&spu->processorStack);

    while (ReadInstruction(outFileDescriptor, spu) == NO_PROCESSOR_ERRORS);

    StackDestruct_ (&spu->processorStack);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (int outFileDescriptor, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    printf ("Reading command: ");

    CommandCode commandCode {};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByNumber (commandCode.opcode);

    if (instruction == NULL){
        RETURN WRONG_INSTRUCTION;
    }

    printf ("%s\n", instruction->instructionName);

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    strcat (commandLine, instruction->instructionName);

    int registerIndex = -1;
    elem_t immedArgument = 0;

    if (commandCode.hasImmedArgument && commandCode.hasRegisterArgument) {
        ReadData (spu, &registerIndex, int);
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, "%s r%cx+%lf\n", instruction->instructionName, registerIndex + 'a', immedArgument);
    }else if (commandCode.hasImmedArgument) {
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, "%s %lf\n", instruction->instructionName, immedArgument);
    }else if (commandCode.hasRegisterArgument){
        ReadData (spu, &registerIndex, int);

        sprintf (commandLine, "%s r%cx\n", instruction->instructionName, registerIndex + 'a');
    }else {
        sprintf (commandLine, "%s\n", instruction->instructionName);
    }

    if (!WriteBuffer (outFileDescriptor, commandLine, (ssize_t) strlen (commandLine))) {
        RETURN OUTPUT_FILE_ERROR;
    }


    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(INSTRUCTION_NAME, ...)   \
            INSTRUCTION_CALLBACK_FUNCTION (INSTRUCTION_NAME) {}

#include "Instructions.def"

#undef INSTRUCTION
