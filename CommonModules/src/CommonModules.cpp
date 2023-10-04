#include "CommonModules.h"

struct AssemblerInstruction AvailableInstructions [] = {
                                INSTRUCTION (hlt, 0, 0), INSTRUCTION (out, -1, 0), INSTRUCTION (in, -2, 0), INSTRUCTION (push, -3, 1), INSTRUCTION (pop, -4, 0), INSTRUCTION (add, 1, 0),
                                INSTRUCTION (sub, 2, 0), INSTRUCTION (mul, 3, 0),  INSTRUCTION (div, 4, 0), INSTRUCTION (sin, 5, 0),   INSTRUCTION (cos, 6, 0),  INSTRUCTION (sqrt, 7, 0)
                            };
