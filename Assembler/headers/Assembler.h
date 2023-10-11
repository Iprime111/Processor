#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "CommonModules.h"
#include "FileIO.h"

ProcessorErrorCode AssembleFile (TextBuffer *text, FileBuffer *file, int binaryDescriptor);

#endif
