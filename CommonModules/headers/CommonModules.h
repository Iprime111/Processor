#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

#include <stddef.h>

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

#define INSTRUCTION(INSTRUCTION_NAME, INSTRUCTION_NUMBER, ARGUMENTS_COUNT, SCANF_SPECIFIERS)    \
            {                                                                                   \
                .instructionName = #INSTRUCTION_NAME,                                           \
                .instruction = INSTRUCTION_NUMBER,                                              \
                .argumentsCount = ARGUMENTS_COUNT,                                              \
                .argumentsScanfSpecifiers = SCANF_SPECIFIERS,                                   \
                .callbackFunction = INSTRUCTION_NAME##Callback                                  \
            }

INSTRUCTION_CALLBACK_FUNCTION (in);
INSTRUCTION_CALLBACK_FUNCTION (hlt);
INSTRUCTION_CALLBACK_FUNCTION (out);
INSTRUCTION_CALLBACK_FUNCTION (push);
INSTRUCTION_CALLBACK_FUNCTION (add);
INSTRUCTION_CALLBACK_FUNCTION (sub);
INSTRUCTION_CALLBACK_FUNCTION (mul);
INSTRUCTION_CALLBACK_FUNCTION (div);
INSTRUCTION_CALLBACK_FUNCTION (sin);
INSTRUCTION_CALLBACK_FUNCTION (cos);
INSTRUCTION_CALLBACK_FUNCTION (sqrt);

const struct AssemblerInstruction AvailableInstructions [] = {
                                INSTRUCTION (hlt,  0, 0, {}), INSTRUCTION (out, -1, 0, {}),  INSTRUCTION (in, -2, 0, {}), INSTRUCTION (push, -3, 1, {"%llu"}), INSTRUCTION (add, 1, 0, {}),
                                INSTRUCTION (sub,  2, 0, {}), INSTRUCTION (mul,  3, 0, {}),  INSTRUCTION (div, 4, 0, {}), INSTRUCTION (sin,   5, 0, {}),      INSTRUCTION (cos, 6, 0, {}),
                                INSTRUCTION (sqrt, 7, 0, {})
                            };


const AssemblerInstruction *FindInstructionByName   (char *name);
const AssemblerInstruction *FindInstructionByNumber (int instruction);

#endif
