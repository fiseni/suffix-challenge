#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

#include <stdlib.h>

// Based on the requirements the part codes are less than 50 characters (ASCII).
// Defining the max as 50 makes it easier to work with arrays and buffer sizes (null terminator).
#define MAX_STRING_LENGTH ((size_t)50)

// Based on the requirements we should ignore part codes with less than 3 characters.
#define MIN_STRING_LENGTH ((size_t)3)

typedef struct Part {
    const char *code;
    size_t codeLength;
    size_t index;                       // Index to the original records. Also used for stable sorting.
} Part;

typedef struct SourceData {
    const Part *masterPartsOriginal;    // Original master parts records, trimmed
    size_t masterPartsOriginalCount;

    const Part *masterPartsAsc;         // Sorted master parts records, uppercased
    size_t masterPartsAscCount;

    const Part *masterPartsNhAsc;       // Sorted master parts records, uppercased, without hyphens (Nh = no hyphens)
    size_t masterPartsNhAscCount;

    const Part *partsOriginal;          // Original parts records, trimmed
    size_t partsOriginalCount;

    const Part *partsAsc;               // Sorted parts records, uppercased
    size_t partsAscCount;
} SourceData;

const SourceData *source_data_read(const char *partsFile, const char *masterPartsFile);
void source_data_clean(const SourceData *data);

#endif
