#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

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
};

typedef ProcessorErrorCode (*callbackFunction_t)();

struct AssemblerInstruction {
    const char *instructionName;
    const long long instruction;

    const unsigned int argumentsCount;
    const char *argumentsScanfSpecifiers [MAX_ARGUMENTS_COUNT];

    callbackFunction_t callbackFunction;
};

#define INSTRUCTION_CALLBACK_FUNCTION(INSTRUCTION_NAME) ProcessorErrorCode INSTRUCTION_NAME##Callback ()

#define INSTRUCTION(INSTRUCTION_NAME, INSTRUCTION_NUMBER, ARGUMENTS_COUNT, SCANF_SPECIFIERS, ...)   \
            INSTRUCTION_CALLBACK_FUNCTION (INSTRUCTION_NAME);

#include "Instructions.h"

#undef INSTRUCTION

const AssemblerInstruction *FindInstructionByName   (char *name);
const AssemblerInstruction *FindInstructionByNumber (int instruction);

#endif
