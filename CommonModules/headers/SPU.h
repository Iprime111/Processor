#ifndef SPU_H_
#define SPU_H_

#include "TextTypes.h"
#include "Stack/Stack.h"

struct SPU {
    FileBuffer *bytecode;
    size_t currentChar = 0;
    Stack processorStack = {};
    elem_t registerValues [4] = {0, 0, 0, 0};
};

#endif
