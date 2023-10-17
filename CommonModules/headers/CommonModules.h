#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

#include "CustomAssert.h"
#include "Logger.h"
#include "SPU.h"

#include <math.h>
#include <stddef.h>

const long long FIXED_FLOAT_PRECISION = 1e3;

const size_t MAX_INSTRUCTION_LENGTH = 100;
const size_t MAX_ARGUMENTS_COUNT = 5;

enum ProcessorErrorCode {
    NO_PROCESSOR_ERRORS = 0,
    WRONG_INSTRUCTION   = 1 << 1,
    BUFFER_ENDED        = 1 << 2,
    NO_BUFFER           = 1 << 3,
    STACK_ERROR         = 1 << 4,
    PROCESSOR_HALT      = 1 << 5,
    TOO_MANY_ARGUMENTS  = 1 << 6,
    TOO_FEW_ARGUMENTS   = 1 << 7,
    OUTPUT_FILE_ERROR   = 1 << 8,
    INPUT_FILE_ERROR    = 1 << 9,
    BLANK_LINE          = 1 << 10,
    NO_PROCESSOR        = 1 << 11,
    WRONG_HEADER        = 1 << 12,
};

enum ArgumentsType {
    NO_ARGUMENTS      = 0,
    IMMED_ARGUMENT    = 1 << 0,
    REGISTER_ARGUMENT = 1 << 1,
};

struct InstructionArguments {
    elem_t immedArgument        = NAN;
    unsigned char registerIndex = REGISTER_COUNT;
};

struct CommandCode {
    unsigned char opcode    : 5;
    unsigned char arguments : 3;
};

typedef ProcessorErrorCode (*callbackFunction_t)(SPU *spu, CommandCode *commandCode);

struct AssemblerInstruction {
    const char *instructionName;
    CommandCode commandCode;

    callbackFunction_t callbackFunction;
};

#define INSTRUCTION_CALLBACK_FUNCTION(INSTRUCTION_NAME) ProcessorErrorCode INSTRUCTION_NAME##Callback (SPU *spu, CommandCode *commandCode)

#define INSTRUCTION(INSTRUCTION_NAME, ...)   \
            INSTRUCTION_CALLBACK_FUNCTION (INSTRUCTION_NAME);

#include "Instructions.def"

#undef INSTRUCTION

const AssemblerInstruction *FindInstructionByName   (char *name);
const AssemblerInstruction *FindInstructionByOpcode (int instruction);

bool CopyVariableValue (void *destination, void *source, size_t size);
char *convertToString ();

#define CheckBuffer(spu)                                                                            \
            do {                                                                                    \
                custom_assert ((spu)->bytecode,                   pointer_is_null, NO_BUFFER);      \
                custom_assert ((spu)->bytecode->buffer_size >= 0, invalid_value,   BUFFER_ENDED);   \
            }while (0)


#define ReadData(spu, destination, type)                                                \
            do {                                                                        \
                char *bufferPointer = (spu)->bytecode->buffer + (spu)->ip;              \
                custom_assert (bufferPointer, pointer_is_null, NO_BUFFER);              \
                if ((ssize_t) (spu)->ip >= (spu)->bytecode->buffer_size) {              \
                    RETURN BUFFER_ENDED;                                                \
                }                                                                       \
                if (!CopyVariableValue (destination, bufferPointer, sizeof (type))) {   \
                    RETURN NO_BUFFER;                                                   \
                }                                                                       \
                (spu)->ip += sizeof (type);                                             \
            }while (0)


#define WriteHeaderField(buffer, header, field, fieldSize)                                                                  \
            do {                                                                                                            \
                WriteDataToBufferErrorCheck ("Error occuried while writing header field " #field " title to listing file",  \
                                    buffer, "\t" #field ":  ", sizeof (#field ":  ") - 1);                                  \
                WriteDataToBufferErrorCheck ("Error occuried while writing header field " #field " to listing file",        \
                                    buffer, &(header)->field,   fieldSize);                                                 \
                WriteDataToBufferErrorCheck ("Error occuried while writing new line " #field " to listing file",            \
                                    buffer, "\n",              1);                                                          \
            } while (0)


#ifndef _NDEBUG
    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define ON_DEBUG(...)
#endif

#endif
