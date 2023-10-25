#ifndef REGISTERS_H_
#define REGISTERS_H_

#include <stddef.h>

const size_t REGISTER_COUNT           = 4;
const size_t MAX_REGISTER_NAME_LENGTH = 7;

struct Register {
    const char   *name  = NULL;
    unsigned char index = REGISTER_COUNT;
};

const Register *FindRegisterByName  (char *name);
const Register *FindRegisterByIndex (unsigned char index);

#endif
