#ifndef STRING_PROCESSING_H_
#define STRING_PROCESSING_H_

#include "FileIO.h"

ssize_t CountWhitespaces (TextLine *line);
ssize_t FindActualStringEnd (TextLine *line);
ssize_t FindActualStringBegin (TextLine *line);

#endif
