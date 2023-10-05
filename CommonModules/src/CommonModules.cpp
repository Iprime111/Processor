#include <string.h>

#include "CustomAssert.h"
#include "CommonModules.h"

#define INSTRUCTION(INSTRUCTION_NAME, INSTRUCTION_NUMBER, ARGUMENTS_COUNT, SCANF_SPECIFIERS, ...)   \
            {                                                                                       \
                .instructionName = #INSTRUCTION_NAME,                                               \
                .instruction = INSTRUCTION_NUMBER,                                                  \
                .argumentsCount = ARGUMENTS_COUNT,                                                  \
                .argumentsScanfSpecifiers = SCANF_SPECIFIERS,                                       \
                .callbackFunction = INSTRUCTION_NAME##Callback,                                     \
            },

static const struct AssemblerInstruction AvailableInstructions [] = {
    #include "Instructions.h"
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

    FindInstruction (AvailableInstructions [instructionIndex].instruction == instruction);
}
