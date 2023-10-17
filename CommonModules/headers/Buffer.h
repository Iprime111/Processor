#ifndef BUFFER_H_
#define BUFFER_H_

#include "CommonModules.h"
#include "MessageHandler.h"
#include <stddef.h>

template <typename T>
struct Buffer {
  	size_t capacity = 0;
  	size_t currentIndex = 0;

  	T *data = NULL;
};

#define WriteDataToBufferErrorCheck(errorMessage, buffer, data, dataSize)                                                       \
			do {																												\
				ProcessorErrorCode _errorCode = NO_PROCESSOR_ERRORS;															\
        		if ((_errorCode = WriteDataToBuffer (buffer, data, dataSize)) != NO_PROCESSOR_ERRORS) {                         \
        		    DestroyBuffer (buffer);                                                                                     \
        		    ErrorFound (_errorCode, errorMessage);                                                                      \
        		}																												\
			} while (0)

template <typename T>
ProcessorErrorCode WriteDataToBuffer (Buffer <T> *buffer, const void *data, size_t dataSize) {
  	PushLog (4);

  	custom_assert (buffer, pointer_is_null, NO_BUFFER);
  	custom_assert (data,   pointer_is_null, NO_BUFFER);

  	if (dataSize > buffer->capacity - buffer->currentIndex) {
    	ErrorFound (BUFFER_ENDED, "Too many data to write into the buffer");
  	}

  	for (size_t dataIndex = 0; dataIndex < dataSize; dataIndex++) {
		buffer->data [buffer->currentIndex++] = ((const T *) data) [dataIndex];
	}

  	RETURN NO_PROCESSOR_ERRORS;
}

template <typename T>
ProcessorErrorCode InitBuffer (Buffer <T> *buffer, size_t capacity) {
	PushLog (4);

	custom_assert (buffer, pointer_is_null, NO_BUFFER);

	buffer->capacity = capacity;
	buffer->data = (char *) calloc (buffer->capacity, sizeof (T));

	if (!buffer->data) {
		ErrorFound (NO_BUFFER, "Error occuried while allocating buffer");
	}

	RETURN NO_PROCESSOR_ERRORS;
}

template <typename T>
ProcessorErrorCode DestroyBuffer (Buffer <T> *buffer) {
	PushLog (4);

	custom_assert (buffer, pointer_is_null, NO_BUFFER);

	free (buffer->data);

	RETURN NO_PROCESSOR_ERRORS;
}


#endif
