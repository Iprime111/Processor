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
static ProcessorErrorCode ReadHeader (Buffer *headerBuffer, SPU *spu) ;

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer *disassemblyBuffer, Buffer *headerBuffer);
static ProcessorErrorCode WriteHeaderData (int outFileDescriptor, Buffer *headerBuffer);

static ProcessorErrorCode CreateDisassemblyBuffers (Buffer *disassemblyBuffer, Buffer *headerBuffer, SPU *spu);
static ProcessorErrorCode DestroyDisassemblyBuffers (Buffer *disassemblyBuffer, Buffer *headerBuffer);

ProcessorErrorCode DisassembleFile (int outFileDescriptor, SPU *spu) {
    PushLog (1);

    CheckBuffer (spu);

    Buffer disassemblyBuffer {};
    Buffer headerBuffer      {};

    ON_DEBUG (PrintSuccessMessage ("Starting disassembly...", NULL));

    CreateDisassemblyBuffers(&disassemblyBuffer, &headerBuffer, spu);

    ProcessorErrorCode errorCode = ReadHeader (&headerBuffer, spu);

    if (errorCode != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        ErrorFound (errorCode, "Invalid header readed");
    }

    while ((errorCode = ReadInstruction (&disassemblyBuffer, spu)) == NO_PROCESSOR_ERRORS);

    if (errorCode != BUFFER_ENDED) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        ErrorFound (errorCode, "Disassembly error occuried");
    }

    if ((errorCode = WriteDisassemblyData (outFileDescriptor, &disassemblyBuffer, &headerBuffer)) != NO_PROCESSOR_ERRORS) {
        errorCode = (ProcessorErrorCode) (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer) | errorCode);
        RETURN errorCode;
    }

    ErrorFound (DestroyDisassemblyBuffers (&disassemblyBuffer, &headerBuffer), "Error occuried while destroying assembly buffers");

    ON_DEBUG (PrintSuccessMessage ("Disassembly finished successfully!", NULL));
    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadHeader (Buffer *headerBuffer, SPU *spu) {
	PushLog (2);

    custom_assert (headerBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (spu,          pointer_is_null, NO_PROCESSOR);

    // Can not use ReadValue macro here 'cause need to control memory allocation
    if ((ssize_t) spu->ip > spu->bytecode->buffer_size) {
        ErrorFound (BUFFER_ENDED, "No commands has been detected");
    }

    #define HEADER_FIELD(FIELD_NAME, FIELD_VALUE, ...)                                                      \
        do {                                                                                                \
            char headerField [sizeof (#FIELD_NAME ": " #FIELD_VALUE) + 1] = "";                             \
            size_t fieldValuePosition = sizeof (#FIELD_NAME ": ") - 1;                                      \
            strcat (headerField, #FIELD_NAME ": ");                                                         \
            while (spu->ip < HEADER_SIZE && spu->bytecode->buffer [spu->ip] != '\0') {                      \
                headerField [fieldValuePosition++] = spu->bytecode->buffer [spu->ip++];                     \
            }                                                                                               \
            if (strcmp (headerField + sizeof (#FIELD_NAME ": ") - 1, #FIELD_VALUE)){                        \
                ErrorFound (WRONG_HEADER, "This binary is incompatible with current processor version");    \
            }                                                                                               \
            headerField [sizeof (headerField) - 2] = '\n';                                                  \
            WriteDataToBuffer (headerBuffer, headerField, strlen (headerField));                            \
            spu->ip++;                                                                                      \
        }while (0);


    #include "AssemblerHeader.def"

    #undef HEADER_FIELD

    spu->ip = HEADER_SIZE;
    WriteDataToBuffer (headerBuffer, "\n\n", 2);

	RETURN NO_PROCESSOR_ERRORS;
}


static ProcessorErrorCode CreateDisassemblyBuffers (Buffer *disassemblyBuffer, Buffer *headerBuffer, SPU *spu) {
    PushLog (3);

    custom_assert (disassemblyBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (headerBuffer     , pointer_is_null, NO_BUFFER);

    //Disassembly format:
    // ip     line
    //0000\t  push ...
    //worst case: 1 byte of instruction = 9 bytes of symbols + '\n' symbol = 10 bytes

    disassemblyBuffer->capacity = (size_t) spu->bytecode->buffer_size * 10 + HEADER_SIZE;
    disassemblyBuffer->data = (char *) calloc (disassemblyBuffer->capacity, sizeof (char));

    if (!disassemblyBuffer->data) {
        ErrorFound (NO_BUFFER, "Unable to create disassembly file buffer");
    }

    // Creating header file
    // size: header size + 2 of '\n' symbols + '\0' symbol

    headerBuffer->capacity = HEADER_SIZE * 2 + 3;
    headerBuffer->data = (char *) calloc (headerBuffer->capacity, sizeof (char));

    if (!headerBuffer->data) {
        ErrorFound (NO_BUFFER, "Unable to create header buffer");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyDisassemblyBuffers (Buffer *disassemblyBuffer, Buffer *headerBuffer) {
    PushLog (3);

    if (disassemblyBuffer) {
        free (disassemblyBuffer->data);
    }

    if (headerBuffer) {
        free (headerBuffer->data);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteHeaderData (int outFileDescriptor, Buffer *headerBuffer) {
    PushLog (2);

    const char HeaderLabel [] = "HEADER:\n";

    if (!WriteBuffer (outFileDescriptor, HeaderLabel, (ssize_t) strlen (HeaderLabel))) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, headerBuffer->data, (ssize_t) headerBuffer->currentIndex)) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing header to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDisassemblyData (int outFileDescriptor, Buffer *disassemblyBuffer, Buffer *headerBuffer) {
    PushLog (2);

    ErrorFound (WriteHeaderData (outFileDescriptor, headerBuffer), "Error occuried while reading header");

    const char DisassemblyLegend[] = " ip \tdisassembly\n";

    if (!WriteBuffer (outFileDescriptor, DisassemblyLegend, (ssize_t) strlen (DisassemblyLegend))) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    if (!WriteBuffer (outFileDescriptor, disassemblyBuffer->data, (ssize_t) disassemblyBuffer->currentIndex)) {
        ErrorFound (OUTPUT_FILE_ERROR, "Error occuried while writing to the disassembly file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadInstruction (Buffer *disassemblyBuffer, SPU *spu) {
    PushLog (2);

    CheckBuffer (spu);

    CommandCode commandCode {0, 0};
    ReadData (spu, &commandCode, CommandCode);

    const AssemblerInstruction *instruction = FindInstructionByOpcode (commandCode.opcode);

    if (!instruction){
        ErrorFound (WRONG_INSTRUCTION, "Wrong instruction readed");
    }

    #ifndef _NDEBUG
        char message [128] = "";
        sprintf (message, "Reading command %s", instruction->instructionName);
        PrintInfoMessage (message, NULL);
    #endif

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
