#include <string.h>

#include "CommonModules.h"
#include "CustomAssert.h"

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

    FindInstruction (strcmp(AvailableInstructions [instructionIndex].instructionName, name) == 0);
}

const AssemblerInstruction *FindInstructionByNumber (int instruction) {
    PushLog (4);

    FindInstruction (AvailableInstructions [instructionIndex].instruction == instruction);
}
