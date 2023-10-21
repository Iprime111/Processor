#ifndef REGISTERS_H_
#define REGISTERS_H_

#include <stddef.h>

const size_t REGISTER_COUNT = 4;

struct Register {
    const char   *name  = NULL;
    unsigned char index = REGISTER_COUNT;
};

const Register *FindRegisterByName  (char *name);
const Register *FindRegisterByIndex (unsigned char index);

#endif