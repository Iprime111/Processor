#ifndef BUFFER_H_
#define BUFFER_H_

#include "CommonModules.h"
#include <stddef.h>

struct Buffer {
  	size_t capacity = 0;
  	size_t currentIndex = 0;
	
  	char *data = NULL;
};

ProcessorErrorCode WriteDataToBuffer (Buffer *buffer, void *data, size_t dataSize);

#endif
