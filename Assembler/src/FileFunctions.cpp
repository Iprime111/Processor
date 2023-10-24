#include <cstddef>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "FileFunctions.h"
#include "AssemblyHeader.h"
#include "CommonModules.h"
#include "CustomAssert.h"
#include "FileIO.h"
#include "Logger.h"
#include "TextTypes.h"

static ProcessorErrorCode WriteHeader    (int binaryDescriptor, int listingDescriptor, size_t commandsCount);
static ProcessorErrorCode WriteDebugInfo (int binaryDescriptor, int listingDescriptor, Buffer <DebugInfoChunk> *debugInfoBuffer);

ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, int binaryDescriptor, int listingDescriptor) {
    PushLog (2);

    custom_assert (binaryBuffer,           pointer_is_null,   NO_BUFFER);
    custom_assert (listingBuffer,          pointer_is_null,   NO_BUFFER);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    ProgramErrorCheck (WriteHeader    (binaryDescriptor, listingDescriptor, debugInfoBuffer->capacity), "Error occuried while writing header");
    ProgramErrorCheck (WriteDebugInfo (binaryDescriptor, listingDescriptor, debugInfoBuffer),           "Error occuried while writing debug info");

    if (!WriteBuffer (binaryDescriptor, binaryBuffer->data, (ssize_t) binaryBuffer->currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to binary file");
    }

    if (listingDescriptor == -1) {
        RETURN NO_PROCESSOR_ERRORS;
    }

    const char *ListingLegend = " ip \topcode\tline \tsource\n";

    if (!WriteBuffer (listingDescriptor, ListingLegend, (ssize_t) strlen (ListingLegend))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to a listing file");
    }

    if (!WriteBuffer (listingDescriptor, listingBuffer->data, (ssize_t) listingBuffer->currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to a listing file");
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteDebugInfo (int binaryDescriptor, int listingDescriptor, Buffer <DebugInfoChunk> *debugInfoBuffer) {
    PushLog (2);

    if (!WriteBuffer (binaryDescriptor, (char *) debugInfoBuffer->data, (ssize_t) (debugInfoBuffer->capacity * sizeof (DebugInfoChunk)))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing debug info to a binary file");
    }

    if (listingDescriptor == -1) {
        RETURN NO_PROCESSOR_ERRORS;
    }

    const char DebugInfoLegend [] = "DEBUG INFO:\n";

    if (!WriteBuffer (listingDescriptor, DebugInfoLegend, sizeof (DebugInfoLegend) - 1)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
    }

    // TODO write debug info to listing

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor, size_t commandsCount) {
    PushLog (2);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    Header header {};
    InitHeader (&header);

    header.commandsCount = commandsCount;

    if (!WriteBuffer (binaryDescriptor, (char *) &header, sizeof (header))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to a binary file");
    }

    if (listingDescriptor == -1) {
        RETURN NO_PROCESSOR_ERRORS;
    }

    const char HeaderLegend [] = "HEADER:\n";

    if (!WriteBuffer (listingDescriptor, HeaderLegend, (ssize_t) sizeof (HeaderLegend) - 1)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
    }

    const size_t ListingHeaderBufferMaxSize = 256; // TODO more precise calculations

    Buffer <char> listingHeaderBuffer {};
    InitBuffer (&listingHeaderBuffer, sizeof (header) * ListingHeaderBufferMaxSize);

    WriteHeaderField (&listingHeaderBuffer, &header, signature,     sizeof (unsigned short));
    WriteHeaderField (&listingHeaderBuffer, &header, version,       sizeof (VERSION) - 1);
    WriteHeaderField (&listingHeaderBuffer, &header, byteOrder,     sizeof (SYSTEM_BYTE_ORDER) - 1);

    // TODO rewrite this macro
    //WriteHeaderField (&listingHeaderBuffer, &header, commandsCount, sizeof (size_t));

    WriteDataToBufferErrorCheck ("Error occuried while writing new line to listing file", &listingHeaderBuffer, "\n\n", strlen ("\n\n"));

    if (!WriteBuffer (listingDescriptor, listingHeaderBuffer.data, (ssize_t) listingHeaderBuffer.currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
    }

    DestroyBuffer (&listingHeaderBuffer);


    RETURN NO_PROCESSOR_ERRORS;
}
