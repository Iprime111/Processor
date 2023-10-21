#ifndef FILE_FUNCTIONS_H_
#define FILE_FUNCTIONS_H_

#include "Buffer.h"
#include "CommonModules.h"

ProcessorErrorCode DeleteExcessWhitespaces (TextBuffer *lines);
ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, int binaryDescriptor, int listingDescriptor);
ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor);

#endif
