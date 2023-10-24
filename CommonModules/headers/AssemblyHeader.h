#ifndef ASSEMBLY_HEADER_H_
#define ASSEMBLY_HEADER_H_

#include <endian.h>

#include "CommonModules.h"

#if BYTE_ORDER == LITTLE_ENDIAN
    const char SYSTEM_BYTE_ORDER [] = "LITTLE ENDIAN";
#else
    const char SYSTEM_BYTE_ORDER [] = "BIG ENDIAN";
#endif

const char VERSION [] = "1.4.0";
const short int DEFAULT_SIGNATURE = 0x4d54;

struct Header {
    unsigned short signature = 0;

    char version   [sizeof (VERSION)]           = "";
    char byteOrder [sizeof (SYSTEM_BYTE_ORDER)] = "";

    size_t commandsCount = 0;
};

#define WriteHeaderField(buffer, header, field, fieldSize)                                                                  \
            do {                                                                                                            \
                WriteDataToBufferErrorCheck ("Error occuried while writing header field " #field " title to listing file",  \
                                    buffer, "\t" #field ":  ", sizeof (#field ":  ") - 1);                                  \
                WriteDataToBufferErrorCheck ("Error occuried while writing header field " #field " to listing file",        \
                                    buffer, &(header)->field,  fieldSize);                                                  \
                WriteDataToBufferErrorCheck ("Error occuried while writing new line " #field " to listing file",            \
                                    buffer, "\n",              1);                                                          \
            } while (0)


ProcessorErrorCode InitHeader  (Header *header);
ProcessorErrorCode CheckHeader (Header *header);

#endif
