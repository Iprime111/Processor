#include "Label.h"
#include "Buffer.h"
#include "CustomAssert.h"
#include <cstring>

ProcessorErrorCode InitLabel (Label *label, char *name, long long address) {
    PushLog (4);

    custom_assert (label, pointer_is_null, WRONG_LABEL);
    custom_assert (name,  pointer_is_null, WRONG_LABEL);

    strcpy (label->name, name);
    label->address = address;

    RETURN NO_PROCESSOR_ERRORS;
}

long long LabelComparator (void *value1, void *value2) {
    PushLog (4);

    custom_assert (value1, pointer_is_null, -1);
    custom_assert (value2, pointer_is_null, -1);

    long long addressDiff = LabelComparatorByAddress (value1, value2);

    if (addressDiff) {
        return addressDiff;
    }

    return LabelComparatorByName (value1, value2);
}

long long LabelComparatorByName (void *value1, void *value2) {
    PushLog (4);

    custom_assert (value1, pointer_is_null, -1);
    custom_assert (value2, pointer_is_null, -1);

    Label *label1 = (Label *) value1;
    Label *label2 = (Label *) value2;

    return strcmp (label1->name, label2->name);
}

long long LabelComparatorByAddress (void *value1, void *value2) {
    PushLog (4);

    custom_assert (value1, pointer_is_null, -1);
    custom_assert (value2, pointer_is_null, -1);

    Label *label1 = (Label *) value1;
    Label *label2 = (Label *) value2;

    long long addressDiff = label1->address - label2->address;

    return addressDiff;
}

