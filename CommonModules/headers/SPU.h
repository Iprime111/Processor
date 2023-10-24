#ifndef SPU_H_
#define SPU_H_

#include <sys/types.h>

#include "TextTypes.h"
#include "Registers.h"
#include "Stack/Stack.h"

const size_t RAM_SIZE = 100;                    // ram and vram sizes
const size_t VRAM_SIZE = 100;

const unsigned int MIN_FREQUENCY = 1;           // minimal and maximal frequency (in MHz) for processor clocking
const unsigned int MAX_FREQUENCY = 4200;
const useconds_t MAX_SLEEP_TIME = (int) 20e6;   // Sleep time when minimal frequency is set

struct DebugInfoChunk {
    size_t address;
    int line;
};

struct SPU {
    FileBuffer bytecode;
    size_t ip = 0;

    Stack processorStack = {};
    Stack callStack      = {};

    elem_t tmpArgument = 0;

    elem_t registerValues [REGISTER_COUNT] = {};

    elem_t ram [RAM_SIZE + VRAM_SIZE] = {};

    useconds_t frequencySleep = 0;
};

#undef REGISTER

inline void ShrinkBytecodeBuffer (SPU *spu, size_t shrinkLength) {
    spu->bytecode.buffer      +=            shrinkLength;
    spu->bytecode.buffer_size -= (ssize_t) (shrinkLength);
    spu->ip = 0;
}

#endif
