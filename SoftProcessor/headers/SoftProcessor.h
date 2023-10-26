#ifndef SOFT_PROCESSOR_H_
#define SOFT_PROCESSOR_H_

#include "CommonModules.h"
#include "TextTypes.h"
#include <SFML/System/Mutex.hpp>

ProcessorErrorCode LaunchProgram (SPU *spu, char *sourceFilename, char *binaryFilename, sf::Mutex *workMutex);

#endif
