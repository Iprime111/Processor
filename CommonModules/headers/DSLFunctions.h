#include "Buffer.h"
#include "CommonModules.h"
#include "Logger.h"
#include "MessageHandler.h"
#include <cmath>

enum ComparisonResult {
    GREATER   = 1 << 0,
    LESS      = 1 << 1,
    EQUAL     = 1 << 2,
};

const double EPS = 1e-5;

inline ComparisonResult CompareValues (elem_t value1, elem_t value2) {
    if (value1 - value2 > EPS) {
        return GREATER;
    } else if (value2 - value1 > EPS) {
        return LESS;
    }else {
        return EQUAL;
    }
}

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

#define PushValue(spu, value)                                                               \
            do {                                                                            \
                if (StackPush_ (&((spu)->processorStack), value) != NO_ERRORS) {            \
                    ErrorFound (STACK_ERROR, "Stack error occuried while pushing value");   \
                }                                                                           \
            }while (0)

#define PopValue(spu, value)                                                                \
            do {                                                                            \
                if (StackPop_ (&((spu)->processorStack), value) != NO_ERRORS) {             \
                    ErrorFound (STACK_ERROR, "Stack error occuried while poping value");    \
                }                                                                           \
            }while (0)

#define Jump(spu, jmpAddress)                                                                   \
            do {                                                                                \
                if ((ssize_t) jmpAddress >= (spu)->bytecode->buffer_size || jmpAddress < 0) {   \
                    ErrorFound (BUFFER_ENDED, "Out of buffer jump attempt");                    \
                }                                                                               \
                (spu)->ip = (size_t) jmpAddress + sizeof (Header);                              \
            } while (0)

#define JumpAssemblerCallback                                                                                                   \
    char currentLabel [128] = "";                                                                                               \
    ON_DEBUG (char message [128] = "");                                                                                         \
    if (sscanf (line->pointer + offset, "%*s %lld", (long long *) &arguments->immedArgument) > 0) {                             \
        isArgumentReadCorrectly = true;                                                                                         \
    } else if (sscanf (line->pointer + offset, "%*s %s", currentLabel) > 0){                                                    \
        Label label {};                                                                                                         \
        InitLabel (&label, currentLabel, -1);                                                                                   \
        Label *foundLabel = FindValueInBuffer (labelsBuffer, &label, LabelComparatorByName);                                    \
        if (foundLabel) {                                                                                                       \
            * (long long *) &arguments->immedArgument = foundLabel->address;                                                    \
        }                                                                                                                       \
        isArgumentReadCorrectly = true;                                                                                         \
    } else {                                                                                                                    \
        ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");                                                               \
    }                                                                                                                           \
    ON_DEBUG (sprintf (message, "Jump found. Jump address: %lld", *(long long *) &arguments->immedArgument));                   \
    ON_DEBUG (PrintInfoMessage (message, NULL));                                                                                \
    instruction->commandCode.arguments = IMMED_ARGUMENT;

#define JumpDisassemblerCallback                                            \
    if (commandCode->arguments & IMMED_ARGUMENT) {                          \
        sprintf (commandLine, " %lld\n", * (long long *) &immedArgument);   \
    }

#define ConditionalJump(spu, comparisonResult)                              \
            do {                                                            \
                elem_t value1 = NAN;                                        \
                elem_t value2 = NAN;                                        \
                PopValue (spu, &value1);                                    \
                PopValue (spu, &value2);                                    \
                long long jmpAddress = 0;                                   \
                ReadData (spu, &jmpAddress, long long);                     \
                if (CompareValues(value2, value1) & (comparisonResult)) {   \
                    Jump (spu, jmpAddress);                                 \
                }                                                           \
            } while (0)
