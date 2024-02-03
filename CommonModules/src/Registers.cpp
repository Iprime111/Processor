#include <string.h>

#include "Registers.h"
#include "Logger.h"

#define REGISTER(NAME, INDEX) {.name=#NAME, .index=INDEX}

static const Register Registers [REGISTER_COUNT] = {
    REGISTER (rax, 0),
    REGISTER (rbx, 1),
    REGISTER (rcx, 2),
    REGISTER (rdx, 3),
    REGISTER (rex, 4),
    REGISTER (rfx, 5),
    REGISTER (rgx, 6),
    REGISTER (rhx, 7),
    REGISTER (rbp, 8),
};

#undef REGISTER

#define FindRegister(predicate)                                                                     \
            do {                                                                                    \
                for (size_t registerIndex = 0; registerIndex < REGISTER_COUNT; registerIndex++) {   \
                    if (predicate) {                                                                \
                        RETURN Registers + registerIndex;                                           \
                    }                                                                               \
                }                                                                                   \
                RETURN NULL;                                                                        \
            }while (0)                                                                              \


const Register *FindRegisterByName  (char *name) {
    PushLog (4);

    FindRegister (!strcmp (Registers [registerIndex].name, name));
}

const Register *FindRegisterByIndex (unsigned char index) {
    PushLog (4);

    FindRegister (Registers [registerIndex].index == index);
}

#undef FindRegister
