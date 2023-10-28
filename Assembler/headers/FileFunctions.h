#ifndef FILE_FUNCTIONS_H_
#define FILE_FUNCTIONS_H_

#include <cstddef>

#include "AssemblyHeader.h"
#include "Buffer.h"
#include "CommonModules.h"

ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, int binaryDescriptor, int listingDescriptor);

#endif
