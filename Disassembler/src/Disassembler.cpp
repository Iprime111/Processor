#include "Disassembler.h"
#include "AssemblyHeader.h"
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
#include "DSLFunctions.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
        ErrorFoundInProgram (errorCode, "Invalid header readed");
    }

    PrintSuccessMessage ("Starting disassembly...", NULL);

    while ((errorCode = ReadInstruction (&disassemblyBuffer, spu)) == NO_PROCESSOR_ERRORS);

    if (errorCode != BUFFER_ENDED) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        ErrorFoundInProgram (errorCode, "Disassembly error occuried");
    }

    if ((errorCode = WriteDisassemblyData (outFileDescriptor, &disassemblyBuffer, &headerBuffer)) != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        RETURN errorCode;
    }

    ErrorFoundInProgram (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer), "Error occuried while destroying assembly buffers");

    PrintSuccessMessage ("Disassembly finished successfully!", NULL);
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadHeader (Buffer <char> *headerBuffer, SPU *spu) {
	PushLog (2);

    custom_assert (headerBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (spu,          pointer_is_null, NO_PROCESSOR);

    Header mainHeader {};
    Header readHeader {};
    InitHeader (&mainHeader);

    ReadData (spu, &readHeader, Header);

    #define CheckHeaderField(field, fieldSize, predicate)                           \
                if (!(predicate)) {                                                 \
                    ErrorFoundInProgram (WRONG_HEADER, "Header field " #field " is wrong");  \
                }else {                                                             \
                    WriteHeaderField (headerBuffer, &readHeader, field, fieldSize); \
                }


    CheckHeaderField (signature, sizeof (unsigned short),        readHeader.signature == mainHeader.signature);
    CheckHeaderField (version,   sizeof (VERSION) - 1,           !strcmp (readHeader.version,   mainHeader.version));
    CheckHeaderField (byteOrder, sizeof (SYSTEM_BYTE_ORDER) - 1, !strcmp (readHeader.byteOrder, mainHeader.byteOrder));

    #undef CheckHeaderField

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

    disassemblyBuffer->capacity = (size_t) spu->bytecode->buffer_size * 10 + sizeof (Header);
    disassemblyBuffer->data = (char *) calloc (disassemblyBuffer->capacity, sizeof (char));

    if (!disassemblyBuffer->data) {
        ErrorFoundInProgram (NO_BUFFER, "Unable to create disassembly file Buffer <char>");
    }

    // Creating header file
    // size: header size + 100

    headerBuffer->capacity = sizeof (Header) + 100;
    headerBuffer->data = (char *) calloc (headerBuffer->capacity, sizeof (char));

    if (!headerBuffer->data) {
        ErrorFoundInProgram (NO_BUFFER, "Unable to create header Buffer <char>");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyDisassemblyBuffers (Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer) {
    PushLog (3);

    if (disassemblyBuffer) {
        free (disassemblyBuffer->data);
    }

    if (headerBuffer) {
        free (headerBuffer->data);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteHeaderData (int outFileDescriptor, Buffer <char> *headerBuffer) {
    PushLog (2);

    const char HeaderLabel [] = "HEADER:\n";

    if (!WriteBuffer (outFileDescriptor, HeaderLabel, (ssize_t) strlen (HeaderLabel))) {
        ErrorFoundInProgram (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, headerBuffer->data, (ssize_t) headerBuffer->currentIndex)) {
        ErrorFoundInProgram (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer <char> *disassemblyBuffer, Buffer <char> *headerBuffer) {
    PushLog (2);

    ErrorFoundInProgram (WriteHeaderData (outFileDescriptor, headerBuffer), "Error occuried while reading header");

    const char DisassemblyLegend[] = " ip \tdisassembly\n";

    if (!WriteBuffer (outFileDescriptor, DisassemblyLegend, (ssize_t) strlen (DisassemblyLegend))) {
        ErrorFoundInProgram (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, disassemblyBuffer->data, (ssize_t) disassemblyBuffer->currentIndex)) {
        ErrorFoundInProgram (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
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
        ErrorFoundInProgram (WRONG_INSTRUCTION, "Wrong instruction readed");
    }

    #ifndef _NDEBUG
        char message [128] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
    #endif

    if ((~instruction->commandCode.arguments) & commandCode.arguments) {
        ErrorFoundInProgram (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments");
    }

    char commandLine [MAX_INSTRUCTION_LENGTH] = "";
    int bytesPrinted = 0;
    sprintf (commandLine, "%.4lu\t%s%n", spu->ip - sizeof (commandCode) - sizeof (Header), instruction->instructionName, &bytesPrinted);

    ProcessorErrorCode errorCode = ReadArguments (instruction, &commandCode, spu, commandLine + bytesPrinted);
    if (errorCode != NO_PROCESSOR_ERRORS) {
        ErrorFoundInProgram (errorCode, "Error occuried while reading function arguments");
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
            ErrorFoundInProgram (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, " %s+%lf\n", registerName, immedArgument);

    }else if (commandCode->arguments & IMMED_ARGUMENT) {
        ReadData (spu, &immedArgument, elem_t);

        sprintf (commandLine, " %lf\n", immedArgument);
    }else if (commandCode->arguments & REGISTER_ARGUMENT){
        ReadData (spu, &registerIndex, char);

        #include "Registers.def"

        if (!registerName) {
            ErrorFoundInProgram (TOO_FEW_ARGUMENTS, "Wrong register name format");
        }

        sprintf (commandLine, " %s\n", registerName);
    }else {
        sprintf (commandLine, "\n");
    }

    #undef REGISTER

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, DISASSEMBLER_CALLBACK)  \
    if (instruction->commandCode.opcode == ((CommandCode) COMMAND_CODE).opcode) {                           \
        DISASSEMBLER_CALLBACK                                                                               \
    }

    #include "Instructions.def"

    #undef INSTRUCTION

    RETURN NO_PROCESSOR_ERRORS;
}

#define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, DISASSEMBLER_CALLBACK)  \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                                                      \
                return NO_PROCESSOR_ERRORS;                                                             \
            }

#include "Instructions.def"

#undef INSTRUCTION
