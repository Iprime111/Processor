#include "Buffer.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "MessageHandler.h"

// TODO buffer constructor

ProcessorErrorCode WriteDataToBuffer (Buffer *buffer, const void *data, size_t dataSize) {
  	PushLog (4);

  	custom_assert (buffer, pointer_is_null, NO_BUFFER);
  	custom_assert (data,   pointer_is_null, NO_BUFFER);

  	if (dataSize > buffer->capacity - buffer->currentIndex) {
    	ErrorFound (BUFFER_ENDED, "Too many data to write into the buffer");
  	}

  	for (size_t dataIndex = 0; dataIndex < dataSize; dataIndex++) {
		buffer->data [buffer->currentIndex++] = ((const char *) data) [dataIndex];
	}

  	RETURN NO_PROCESSOR_ERRORS;
}
