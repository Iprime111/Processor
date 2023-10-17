#ifndef ASSEMBLY_HEADER_H_
#define ASSEMBLY_HEADER_H_

#include "CommonModules.h"

#include <endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
    const char SYSTEM_BYTE_ORDER [] = "LITTLE ENDIAN";
#else
    const char SYSTEM_BYTE_ORDER [] = "BIG ENDIAN";
#endif

const char VERSION [] = "1.2.0";
const short int DEFAULT_SIGNATURE = 19796;

struct Header {
    unsigned short signature = 0;
    char version [sizeof (VERSION)];
    char byteOrder [sizeof (SYSTEM_BYTE_ORDER)];
};

ProcessorErrorCode InitHeader (Header *header);

#endif
