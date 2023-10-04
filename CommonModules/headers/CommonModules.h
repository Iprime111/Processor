#ifndef COMMON_MODULES_H_
#define COMMON_MODULES_H_

#include <stddef.h>

enum ProcessorErrorCode {
    NO_PROCESSOR_ERRORS = 0,
    WRONG_INSTRUCTION   = 1 << 1,
};

typedef ProcessorErrorCode (*callbackFunction_t)();

struct AssemblerInstruction {
    char *instructionName;
    int instruction;

    unsigned int argumentsCount;

    callbackFunction_t callbackFunction;
};

#define INSTRUCTION_CALLBACK_FUNCTION(INSTRUCTION_NAME) ProcessorErrorCode INSTRUCTION_NAME##Callback ()

#define INSTRUCTION(INSTRUCTION_NAME, INSTRUCTION_NUMBER, ARGUMENTS_COUNT)  \
            {                                                               \
                .instructionName = #INSTRUCTION_NAME,                       \
                .instruction = INSTRUCTION_NUMBER,                          \
                .argumentsCount = ARGUMENTS_COUNT,                          \
                .callbackFunction = INSTRUCTION_NAME##Callback              \
            }

INSTRUCTION_CALLBACK_FUNCTION (in);
INSTRUCTION_CALLBACK_FUNCTION (hlt);
INSTRUCTION_CALLBACK_FUNCTION (out);
INSTRUCTION_CALLBACK_FUNCTION (push);
INSTRUCTION_CALLBACK_FUNCTION (pop);
INSTRUCTION_CALLBACK_FUNCTION (add);
INSTRUCTION_CALLBACK_FUNCTION (sub);
INSTRUCTION_CALLBACK_FUNCTION (mul);
INSTRUCTION_CALLBACK_FUNCTION (div);
INSTRUCTION_CALLBACK_FUNCTION (sin);
INSTRUCTION_CALLBACK_FUNCTION (cos);
INSTRUCTION_CALLBACK_FUNCTION (sqrt);

#endif
