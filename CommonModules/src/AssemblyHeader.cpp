#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include <AssemblyHeader.h>
#include <cstring>

ProcessorErrorCode InitHeader (Header *header) {
    PushLog (4);
    custom_assert (header, pointer_is_null, WRONG_HEADER);

    strcpy (header->version,   VERSION);
    strcpy (header->byteOrder, SYSTEM_BYTE_ORDER);
    header->signature = DEFAULT_SIGNATURE;

    RETURN NO_PROCESSOR_ERRORS;
}
