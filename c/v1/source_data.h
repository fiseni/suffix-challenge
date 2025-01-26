#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

#include "common.h"

// Based on the requirements the part codes are less than 50 characters (ASCII).
// Defining the max as 50 makes it easier to work with arrays and buffer sizes (null terminator).
#define MAX_STRING_LENGTH ((size_t)50)

// Based on the requirements we should ignore part codes with less than 3 characters.
#define MIN_STRING_LENGTH ((size_t)3)

typedef struct MasterPart {
    const char *code;
    size_t codeLength;
    size_t index;
} MasterPart;

typedef struct Part {
    const char *code;
    size_t codeLength;
    size_t index;
} Part;

typedef struct SourceData {
    const MasterPart *masterPartsOriginal;
    size_t masterPartsOriginalCount;
    const MasterPart *masterPartsAsc;
    size_t masterPartsAscCount;
    const MasterPart *masterPartsAscNh;
    size_t masterPartsAscNhCount;

    const Part *partsOriginal;
    size_t partsOriginalCount;
    const Part *partsAsc;
    size_t partsAscCount;
} SourceData;

const SourceData *source_data_read(const char *partsFile, const char *masterPartsFile);
void source_data_clean(const SourceData *data);

#endif
