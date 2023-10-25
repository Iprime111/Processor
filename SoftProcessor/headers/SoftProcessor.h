#ifndef SOFT_PROCESSOR_H_
#define SOFT_PROCESSOR_H_

#include "CommonModules.h"
#include "TextTypes.h"

ProcessorErrorCode LaunchProgram (SPU *spu, char *sourceFilename, char *binaryFilename);

#endif
