#include <string.h>

#include "CustomAssert.h"
#include "Logger.h"
#include "AssemblyHeader.h"
#include "DSLFunctions.h"

static bool DebugMode = false;

ProcessorErrorCode CheckHeader (Header *header) {
	PushLog (2);

    custom_assert (header, pointer_is_null, WRONG_HEADER);

    Header mainHeader {};
    InitHeader (&mainHeader);

    #define CheckHeaderField(field, fieldSize, predicate)                                   \
                if (!(predicate)) {                                                         \
                    ProgramErrorCheck (WRONG_HEADER, "Header field " #field " is wrong"); \
                }


    CheckHeaderField (signature, sizeof (unsigned short),        header->signature == mainHeader.signature);
    CheckHeaderField (version,   sizeof (VERSION) - 1,           !strcmp (header->version,   mainHeader.version));
    CheckHeaderField (byteOrder, sizeof (SYSTEM_BYTE_ORDER) - 1, !strcmp (header->byteOrder, mainHeader.byteOrder));

    #undef CheckHeaderField

	RETURN NO_PROCESSOR_ERRORS;
}

ProcessorErrorCode InitHeader (Header *header) {
    PushLog (4);
    custom_assert (header, pointer_is_null, WRONG_HEADER);

    strcpy (header->version,   VERSION);
    strcpy (header->byteOrder, SYSTEM_BYTE_ORDER);
    header->signature = DEFAULT_SIGNATURE;

    header->hasDebugInfo = DebugMode;

    RETURN NO_PROCESSOR_ERRORS;
}

void SetDebugMode (bool debugMode) {
    PushLog (4);

    DebugMode = debugMode;

    RETURN;
}

bool IsDebugMode () {
    PushLog (4);
    RETURN DebugMode;
}
