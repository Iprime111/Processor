#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

#include <stddef.h>
#include "CustomAssert.h"
#include "Logger.h"
#include "SPU.h"

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
};

enum ArgumentsType {
    IMMED_ARGUMENT    = 1 << 0,
    REGISTER_ARGUMENT = 1 << 1,
};

struct CommandCode {
    char opcode    : 4;
    char arguments : 4;
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

#define CheckBuffer(spu)                                                                            \
            do {                                                                                    \
                custom_assert ((spu)->bytecode,                   pointer_is_null, NO_BUFFER);      \
                custom_assert ((spu)->bytecode->buffer_size >= 0, invalid_value,   BUFFER_ENDED);   \
            }while (0)


#define ReadData(spu, destination, type)                                        \
        do {                                                                    \
            char *bufferPointer = (spu)->bytecode->buffer + (spu)->ip;          \
            custom_assert (bufferPointer, pointer_is_null, NO_BUFFER);          \
            if ((ssize_t) (spu)->ip >= (spu)->bytecode->buffer_size) {          \
                RETURN BUFFER_ENDED;                                            \
            }                                                                   \
            CopyVariableValue (destination, bufferPointer, sizeof (type));      \
            (spu)->ip += sizeof (type);                                         \
        }while (0)


#ifndef _NDEBUG
    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define ON_DEBUG(...)
#endif

#endif
