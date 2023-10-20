#ifndef SPU_H_
#define SPU_H_

#include "TextTypes.h"
#include "Stack/Stack.h"

const size_t RAM_SIZE = 100;
const size_t VRAM_SIZE = 100;

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
    elem_t tmpArgument = 0;

    elem_t registerValues [REGISTER_COUNT] = {
        #include "Registers.def"
    };

    elem_t ram [RAM_SIZE + VRAM_SIZE] = {};
};

#undef REGISTER

#endif
