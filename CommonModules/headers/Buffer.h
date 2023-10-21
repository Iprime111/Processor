#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>
#include <stddef.h>

#include "CommonModules.h"
#include "Logger.h"
#include "MessageHandler.h"

typedef long long comparator_t (void *, void *);

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
        		    ProgramErrorCheck (_errorCode, errorMessage);                                                             	\
        		}																												\
			} while (0)

template <typename T>
ProcessorErrorCode WriteDataToBuffer (Buffer <T> *buffer, const void *data, size_t dataSize) {
  	PushLog (4);

  	custom_assert (buffer, pointer_is_null, NO_BUFFER);
  	custom_assert (data,   pointer_is_null, NO_BUFFER);

  	if (dataSize > buffer->capacity - buffer->currentIndex) {
    	ProgramErrorCheck (BUFFER_ENDED, "Too many data to write into the buffer");
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
	buffer->data = (T *) calloc (buffer->capacity, sizeof (T));

	if (!buffer->data) {
		ProgramErrorCheck (NO_BUFFER, "Error occuried while allocating buffer");
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

template <typename T>
T *FindValueInBuffer (Buffer <T> *buffer, T *value, comparator_t *comparator) {
	PushLog (3);

	custom_assert (buffer, 	   pointer_is_null, NULL);
	custom_assert (comparator, pointer_is_null, NULL);
	custom_assert (value,      pointer_is_null, NULL);

	for (size_t index = 0; index < buffer->currentIndex; index++) {
		if (!comparator (buffer->data + index, value)) {
			RETURN buffer->data + index;
		}
	}

	RETURN NULL;
}


#endif
