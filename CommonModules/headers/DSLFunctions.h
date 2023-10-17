#include "CommonModules.h"
#include "MessageHandler.h"
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

#define Jump(spu, jmpAddress)                                                   \
            do {                                                                \
                if ((ssize_t) jmpAddress >= (spu)->bytecode->buffer_size) {     \
                    ErrorFound (BUFFER_ENDED, "Out of buffer jump attempt");    \
                }                                                               \
                (spu)->ip = jmpAddress + sizeof (Header);                       \
            } while (0)

#define JumpAssemblerCallback                                                                                                   \
    if (sscanf (line->pointer + offset, "%*s %llu", (unsigned long long *) &arguments->immedArgument) > 0) {                    \
        ON_DEBUG (char message [128] = "");                                                                                     \
        ON_DEBUG (sprintf (message, "Jump found. Jump address: %llu", *(unsigned long long *) &arguments->immedArgument));      \
        ON_DEBUG (PrintInfoMessage (message, NULL));                                                                            \
    } else if (sscanf (line->pointer + offset, "%*s %s", (unsigned long long *) &arguments->immedArgument) > 0){                \
        /*TODO labels*/                                                                                                         \
    } else {                                                                                                                    \
        ErrorFound (TOO_FEW_ARGUMENTS, "Wrong arguments format");                                                               \
    }
