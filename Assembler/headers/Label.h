#ifndef LABEL_H_
#define LABEL_H_

#include "CommonModules.h"
#include "CustomAssert.h"
#include "Logger.h"
#include <cstring>

#include <stddef.h>

const size_t LABEL_NAME_LENGTH = 128;

struct Label {
    char name [LABEL_NAME_LENGTH] = "";
    long long address = -1;
};

ProcessorErrorCode InitLabel (Label *label, char *name, long long address);

long long LabelComparator          (void *value1, void *value2);
long long LabelComparatorByName    (void *value1, void *value2);
long long LabelComparatorByAddress (void *value1, void *value2);

#endif
