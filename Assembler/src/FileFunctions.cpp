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

ProcessorErrorCode DeleteExcessWhitespaces (TextBuffer *lines) {
    PushLog (3);

    custom_assert (lines,        pointer_is_null, NO_BUFFER);
    custom_assert (lines->lines, pointer_is_null, NO_BUFFER);

    for (size_t lineIndex = 0; lineIndex < lines->line_count; lineIndex++) {
        TextLine *currentLine = lines->lines + lineIndex;

        bool metWhitespace = false;
        char *writingPointer = currentLine->pointer;

        for (size_t symbolIndex = 0; symbolIndex < currentLine->length; symbolIndex++) {
            if (isspace (currentLine->pointer [symbolIndex])) {
                if (!metWhitespace) {
                    metWhitespace = true;
                } else {
                    continue;
                }

            } else {
                metWhitespace = false;
            }

            *writingPointer = currentLine->pointer [symbolIndex];
            writingPointer++;
        }

        *writingPointer = '\0';
        currentLine->length = (size_t) (writingPointer - currentLine->pointer);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

ProcessorErrorCode WriteDataToFiles (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, int binaryDescriptor, int listingDescriptor) {
    PushLog (2);

    custom_assert (binaryBuffer,           pointer_is_null,   NO_BUFFER);
    custom_assert (listingBuffer,          pointer_is_null,   NO_BUFFER);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    ProgramErrorCheck (WriteHeader (binaryDescriptor, listingDescriptor), "Error occuried while writing header");

    if (!WriteBuffer (binaryDescriptor, binaryBuffer->data, (ssize_t) binaryBuffer->currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to binary file");
    }

    if (listingDescriptor != -1) {
        const char *ListingLegend = " ip \topcode\tline \tsource\n";

        if (!WriteBuffer (listingDescriptor, ListingLegend, (ssize_t) strlen (ListingLegend))) {
            ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to listing file");
        }

        if (!WriteBuffer (listingDescriptor, listingBuffer->data, (ssize_t) listingBuffer->currentIndex)) {
            ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing to listing file");
        }
    }

    RETURN NO_PROCESSOR_ERRORS;
}

ProcessorErrorCode WriteHeader (int binaryDescriptor, int listingDescriptor) {
    PushLog (2);
    custom_assert (binaryDescriptor != -1, invalid_arguments, OUTPUT_FILE_ERROR);

    Header header {};
    InitHeader (&header);

    if (!WriteBuffer (binaryDescriptor, (char *) &header, sizeof (header))) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to binary file");
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

    WriteHeaderField (&listingHeaderBuffer, &header, signature, sizeof (unsigned short));
    WriteHeaderField (&listingHeaderBuffer, &header, version,   sizeof (VERSION) - 1);
    WriteHeaderField (&listingHeaderBuffer, &header, byteOrder, sizeof (SYSTEM_BYTE_ORDER) - 1);

    WriteDataToBufferErrorCheck ("Error occuried while writing new line to listing file", &listingHeaderBuffer, "\n\n", strlen ("\n\n"));

    if (!WriteBuffer (listingDescriptor, listingHeaderBuffer.data, (ssize_t) listingHeaderBuffer.currentIndex)) {
        ProgramErrorCheck (OUTPUT_FILE_ERROR, "Error occuried while writing header to listing file");
    }

    DestroyBuffer (&listingHeaderBuffer);


    RETURN NO_PROCESSOR_ERRORS;
}
