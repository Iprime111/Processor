#include "Disassembler.h"
#include "AssemblyHeader.h"
#include "Buffer.h"
#include "ColorConsole.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "Registers.h"
#include "Stack/StackPrintf.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "SPU.h"
#include "DSLFunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

static ProcessorErrorCode ReadInstruction (Buffer <char> *disassemblyBuffer, SPU *spu);
static ProcessorErrorCode ReadArguments (const AssemblerInstruction *instruction, CommandCode *commandCode, SPU *spu, char *commandLine);
static ProcessorErrorCode ReadHeader (Buffer <char> *headerBuffer, SPU *spu) ;

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer);
static ProcessorErrorCode WriteHeaderData (int outFileDescriptor, Buffer <char> *headerBuffer);

static ProcessorErrorCode CreateDisassemblyBuffers (Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer, SPU *spu);
static ProcessorErrorCode DestroyDisassemblyBuffers (Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer);

ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    CheckBuffer (spu);

    Buffer <char> disassemblyBuffer {};
    Buffer <char> headerBuffer      {};

    CreateDisassemblyBuffers (&disassemblyBuffer, &headerBuffer, spu);

    PrintSuccessMessage ("Reading header...", NULL);
    ProcessorErrorCode errorCode = ReadHeader (&headerBuffer, spu);

    if (errorCode != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        ProgramErrorCheck (errorCode, "Invalid header readed");
    }

    PrintSuccessMessage ("Starting disassembly...", NULL);

    while ((errorCode = ReadInstruction (&disassemblyBuffer, spu)) == NO_PROCESSOR_ERRORS);

    if (errorCode != BUFFER_ENDED) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        ProgramErrorCheck (errorCode, "Disassembly error occuried");
    }

    if ((errorCode = WriteDisassemblyData (outFileDescriptor, &disassemblyBuffer, &headerBuffer)) != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        RETURN errorCode;
    }

    ProgramErrorCheck (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer), "Error occuried while destroying assembly buffers");

    PrintSuccessMessage ("Disassembly finished successfully!", NULL);
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadHeader (Buffer <char> *headerBuffer, SPU *spu) {
	PushLog (2);

    custom_assert (headerBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (spu,          pointer_is_null, NO_PROCESSOR);

    Header readHeader {};
    ReadData (spu, &readHeader, Header);

    ProgramErrorCheck (CheckHeader (&readHeader), "Header is corrupted");

    WriteHeaderField (headerBuffer, &readHeader, signature,     sizeof (unsigned short));
    WriteHeaderField (headerBuffer, &readHeader, version,       sizeof (VERSION) - 1);
    WriteHeaderField (headerBuffer, &readHeader, byteOrder,     sizeof (SYSTEM_BYTE_ORDER) - 1);

    // TODO
    //WriteHeaderField (headerBuffer, &readHeader, commandsCount, sizeof (size_t));

    ShrinkBytecodeBuffer (spu, sizeof (Header) + sizeof (DebugInfoChunk) * readHeader.commandsCount);

    WriteDataToBufferErrorCheck ("Error occuried while writing new line to header buffer", headerBuffer, "\n\n", 2);

	RETURN NO_PROCESSOR_ERRORS;
}


static ProcessorErrorCode CreateDisassemblyBuffers (Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer, SPU *spu) {
    PushLog (3);

    custom_assert (disassemblyBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (headerBuffer     , pointer_is_null, NO_BUFFER);

    //Disassembly format:
    // ip     line
    //0000\t  push ...
    //worst case: 1 byte of instruction = 9 bytes of symbols + '\n' symbol = 10 bytes
    ProgramErrorCheck (InitBuffer (disassemblyBuffer, (size_t) spu->bytecode.buffer_size * 10 + sizeof (Header)), "Unable to create disassembly buffer");

    // Creating header file
    // size: header size + 100
    ProgramErrorCheck (InitBuffer (headerBuffer, sizeof (Header) + 100), "Unable to create header buffer");

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyDisassemblyBuffers (Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer) {
    PushLog (3);

    DestroyBuffer (disassemblyBuffer);
    DestroyBuffer (headerBuffer);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteHeaderData (int outFileDescriptor, Buffer <char> *headerBuffer) {
    PushLog (2);

    const char HeaderLabel [] = "HEADER:\n";

    if (!WriteBuffer (outFileDescriptor, HeaderLabel, (ssize_t) strlen (HeaderLabel))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, headerBuffer->data, (ssize_t) headerBuffer->currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer) {
    PushLog (2);

    ProgramErrorCheck (WriteHeaderData (outFileDescriptor, headerBuffer), "Error occuried while reading header");

    const char DisassemblyLegend[] = " ip \tdisassembly\n";

    if (!WriteBuffer (outFileDescriptor, DisassemblyLegend, (ssize_t) strlen (DisassemblyLegend))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, disassemblyBuffer->data, (ssize_t) disassemblyBuffer->currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (Buffer <char> *disassemblyBuffer, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    CommandCode commandCode {0, 0};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

    if (!instruction){
        ProgramErrorCheck (WRONG_INSTRUCTION, "Wrong instruction readed");
    }

    ON_DEBUG(
        char message [128] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
    )

    if ((~instruction->commandCode.arguments) & commandCode.arguments) {
        ProgramErrorCheck (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
    }

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    int bytesPrinted = 0;
    sprintf (commandLine, "%.4lu\t%s%n", spu->ip - sizeof (commandCode), instruction->instructionName, &bytesPrinted);

    ProcessorErrorCode errorCode = ReadArguments (instruction, &commandCode, spu, commandLine + bytesPrinted);
    if (errorCode != NO_PROCESSOR_ERRORS) {
        ProgramErrorCheck (errorCode, "Error occuried while reading function arguments");
    }

    RETURN WriteDataToBuffer (disassemblyBuffer, commandLine, strlen (commandLine));
}

static ProcessorErrorCode ReadArguments (const AssemblerInstruction *instruction, CommandCode *commandCode, SPU *spu, char *commandLine) {
    PushLog (3);

    unsigned char registerIndex = REGISTER_COUNT;
    elem_t immedArgument = 0;

    sprintf (commandLine++, " ");

    if (commandCode->arguments & MEMORY_ARGUMENT) {
        sprintf (commandLine++, "[");
    }

    int printedSymbols = 0;

    if (commandCode->arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT) || commandCode->arguments == (IMMED_ARGUMENT | REGISTER_ARGUMENT | MEMORY_ARGUMENT)) {
        ReadData (spu, &registerIndex, unsigned char);
        ReadData (spu, &immedArgument, elem_t);

        const Register *foundRegister = FindRegisterByIndex (registerIndex);

        if (!foundRegister) {
            ProgramErrorCheck (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, "%s+%lf%n", foundRegister->name, immedArgument, &printedSymbols);

    }else if (commandCode->arguments & IMMED_ARGUMENT) {
        ReadData (spu, &immedArgument, elem_t);
        sprintf (commandLine, "%lf%n", immedArgument, &printedSymbols);

    }else if (commandCode->arguments & REGISTER_ARGUMENT){
        ReadData (spu, &registerIndex, unsigned char);

        const Register *foundRegister = FindRegisterByIndex (registerIndex);

        if (!foundRegister) {
            ProgramErrorCheck (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, "%s%n", foundRegister->name, &printedSymbols);

    }

    #undef REGISTER

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, DISASSEMBLER_CALLBACK)  \
    if (instruction->commandCode.opcode == ((CommandCode) COMMAND_CODE).opcode) {                           \
        DISASSEMBLER_CALLBACK                                                                               \
    }

    #include "Instructions.def"

    #undef INSTRUCTION

    commandLine += printedSymbols;

    if (commandCode->arguments & MEMORY_ARGUMENT) {
        sprintf (commandLine++, "]");
    }

    sprintf (commandLine, "\n");

    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, DISASSEMBLER_CALLBACK)  \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                                      \
                return NO_PROCESSOR_ERRORS;                                                             \
            }

#include "Instructions.def"

#undef INSTRUCTION
