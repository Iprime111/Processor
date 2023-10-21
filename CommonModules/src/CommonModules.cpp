#include <string.h>

#include "CustomAssert.h"
#include "CommonModules.h"

#define INSTRUCTION(NAME, COMMAND_CODE, ...)        \
            {                                       \
                .instructionName = #NAME,           \
                .commandCode = COMMAND_CODE,        \
                .callbackFunction = NAME##Callback, \
            },

static const struct AssemblerInstruction AvailableInstructions [] = {
    #include "Instructions.def"
};
#undef INSTRUCTION

#define  FindInstruction(predicate)                                                                                                                         \
            do {                                                                                                                                            \
                for (size_t instructionIndex = 0; instructionIndex < sizeof (AvailableInstructions) / sizeof (AssemblerInstruction); instructionIndex++) {  \
                    if (predicate) {                                                                                                                        \
                        RETURN AvailableInstructions + instructionIndex;                                                                                    \
                    }                                                                                                                                       \
                }                                                                                                                                           \
                RETURN NULL;                                                                                                                                \
            }while (0)                                                                                                                                      \


const AssemblerInstruction *FindInstructionByName (char *name) {
    PushLog (4);

    FindInstruction (strcmp (AvailableInstructions [instructionIndex].instructionName, name) == 0);
}

const AssemblerInstruction *FindInstructionByOpcode (int instruction) {
    PushLog (4);

    FindInstruction ((int) AvailableInstructions [instructionIndex].commandCode.opcode == instruction);
}

bool CopyVariableValue (void *destination, void *source, size_t size) {
    PushLog (4);

    custom_assert (destination, pointer_is_null, false);
    custom_assert (source,      pointer_is_null, false);

    if (destination == source) {
        RETURN true;
    }

    char *destinationPointer = (char *) destination;
    char *sourcePointer = (char *) source;

    for (size_t dataChunk = 0; dataChunk < size; dataChunk++) {
        *(destinationPointer + dataChunk) = *(sourcePointer + dataChunk);
    }

    RETURN true;
}
