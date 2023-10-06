#include <string.h>

#include "CustomAssert.h"
#include "CommonModules.h"

#define INSTRUCTION(NAME, OPCODE, ...)              \
            {                                       \
                .instructionName = #NAME,           \
                .commandCode = {OPCODE, 0, 0},      \
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

const AssemblerInstruction *FindInstructionByNumber (int instruction) {
    PushLog (4);

    FindInstruction ((int) AvailableInstructions [instructionIndex].commandCode.opcode == instruction);
}
