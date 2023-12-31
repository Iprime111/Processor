#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

#include <math.h>
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
    NO_PROCESSOR        = 1 << 11,
    WRONG_HEADER        = 1 << 12,
    WRONG_LABEL         = 1 << 13,
    WRONG_FREQUENCY     = 1 << 14,
    WRONG_LINE          = 1 << 15,
    RESET_PROCESSOR     = 1 << 16,
    FORK_ERROR          = 1 << 17,
    WRONG_ADDRESS       = 1 << 18,
};

enum ArgumentsType {
    NO_ARGUMENTS      = 0,
    IMMED_ARGUMENT    = 1 << 0,
    REGISTER_ARGUMENT = 1 << 1,
    MEMORY_ARGUMENT   = 1 << 2,
};

struct InstructionArguments {
    elem_t immedArgument        = NAN;
    unsigned char registerIndex = REGISTER_COUNT;
};

struct CommandCode {
    unsigned char opcode    : 5;
    unsigned char arguments : 3;
};

typedef ProcessorErrorCode (*callbackFunction_t)(SPU *spu, CommandCode *commandCode, elem_t *argument);

struct AssemblerInstruction {
    const char *instructionName;
    CommandCode commandCode;

    callbackFunction_t callbackFunction;
};

#define INSTRUCTION_CALLBACK_FUNCTION(INSTRUCTION_NAME) ProcessorErrorCode INSTRUCTION_NAME##Callback (SPU *spu, CommandCode *commandCode, elem_t *argument)

#define INSTRUCTION(INSTRUCTION_NAME, ...)   \
            INSTRUCTION_CALLBACK_FUNCTION (INSTRUCTION_NAME);

#include "Instructions.def"

#undef INSTRUCTION

const AssemblerInstruction *FindInstructionByName   (char *name);
const AssemblerInstruction *FindInstructionByOpcode (int instruction);

bool CopyVariableValue (void *destination, void *source, size_t size);

#ifndef _NDEBUG
    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define ON_DEBUG(...)
#endif

#endif
