#ifndef SPU_H_
#define SPU_H_

#include "TextTypes.h"
#include "Stack/Stack.h"
#include <sys/types.h>

const size_t RAM_SIZE = 100;           // ram and vram arrays sizes
const size_t VRAM_SIZE = 100;

const unsigned int MIN_FREQUENCY = 1;           // minimal and maximal frequency (in MHz) for processor clocking
const unsigned int MAX_FREQUENCY = 4200;
const useconds_t MAX_SLEEP_TIME = (int) 20e6;   // Sleep time when minimal frequency is set

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

    useconds_t frequencySleep = 0;
};

#undef REGISTER

#endif
