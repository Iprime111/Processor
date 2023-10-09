#ifndef SPU_H_
#define SPU_H_

#include "TextTypes.h"
#include "Stack/Stack.h"

// TODO CheckRegisterIndex(char registerIndex) function
const char REGISTER_COUNT = 4;

struct SPU {
    FileBuffer *bytecode;
    size_t currentChar = 0; // TODO ip
    Stack processorStack = {};
    elem_t registerValues [REGISTER_COUNT] = {0, 0, 0, 0};
};

#endif
