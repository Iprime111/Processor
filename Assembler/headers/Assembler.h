#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "CommonModules.h"
#include "FileIO.h"

ProcessorErrorCode AssembleFile (TextBuffer *file, int outFileDescriptor);

#endif
