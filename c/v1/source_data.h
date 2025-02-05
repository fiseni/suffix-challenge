#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

#include <stdlib.h>

// Based on the requirements the part codes are less than 50 characters (ASCII).
// Defining the max as 50 makes it easier to work with arrays and buffer sizes (null terminator).
#define MAX_STRING_LENGTH ((size_t)50)

// Based on the requirements we should ignore part codes with less than 3 characters.
#define MIN_STRING_LENGTH ((size_t)3)

typedef struct StringAllocationBlock {
    const void *blockParts;
    const void *blockMasterParts;
} StringAllocationBlock;

typedef struct Part {
    StringView code;
    StringView *codeOriginal;
} Part;

typedef struct SourceData {
    const StringView *partCodesOriginal;    // Original parts records, trimmed
    size_t partsCount;

    const StringView *mpCodesOriginal;      // Original master parts records, trimmed
    size_t mpCount;

    const Part *partsAsc;                   // Sorted parts records, uppercased
    size_t partsAscCount;

    const Part *mpAsc;                      // Sorted master parts records, uppercased
    size_t mpAscCount;

    const Part *mpNhAsc;                    // Sorted master parts records, uppercased, without hyphens (Nh = no hyphens)
    size_t mpNhAscCount;

    StringAllocationBlock stringBlock;
} SourceData;

void source_data_load(SourceData *data, const char *partsFile, const char *masterPartsFile);
void source_data_clean(const SourceData *data);

#endif
