#ifndef SPU_H_
#define SPU_H_

#include "TextTypes.h"
#include "Stack/Stack.h"

#define REGISTER(...) 1+
const char REGISTER_COUNT =
    #include "Registers.def"
    0;

#undef REGISTER

#define REGISTER(...) 0,


struct SPU {
    FileBuffer *bytecode;
    size_t ip = 0;

    Stack processorStack = {};

    elem_t registerValues [REGISTER_COUNT] = {
        #include "Registers.def"
    };
};

#undef REGISTER

#endif
