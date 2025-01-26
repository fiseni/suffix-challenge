#include "common.h"
#include "source_data.h"

static void build_parts(const char *partsPath, SourceData *data);
static void build_masterParts(const char *masterPartsPath, SourceData *data);
static int compare_mp_by_code_length_asc(const void *a, const void *b);
static int compare_part_by_code_length_asc(const void *a, const void *b);

const SourceData *source_data_read(const char *partsFile, const char *masterPartsFile) {
    SourceData *data = (SourceData *)malloc(sizeof(*data));
    CHECK_ALLOC(data);

    build_parts(partsFile, data);
    build_masterParts(masterPartsFile, data);
    return data;
}

void source_data_clean(const SourceData *data) {
    // All strings are allocated from a single block
    free((void *)data->masterPartsOriginal->code);
    free((void *)data->partsOriginal->code);

    free((void *)data->masterPartsOriginal);
    free((void *)data->masterPartsAsc);
    free((void *)data->masterPartsAscNh);
    free((void *)data->partsOriginal);
    free((void *)data->partsAsc);
    free((void *)data);
}

static void build_parts(const char *partsPath, SourceData *data) {
    size_t lineCount;
    size_t contentSize;
    char *block = read_file(partsPath, 2, &contentSize, &lineCount);

    Part *parts = malloc(lineCount * sizeof(*parts));
    CHECK_ALLOC(parts);
    Part *partsAsc = malloc(lineCount * sizeof(*partsAsc));
    CHECK_ALLOC(partsAsc);

    size_t partsIndex = 0;
    size_t stringStartIndex = 0;
    size_t blockIndexUpper = contentSize;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] != '\n') {
            continue;
        }
        block[i] = '\0';
        size_t length = i - stringStartIndex;
        if (i > 0 && block[i - 1] == '\r') {
            block[i - 1] = '\0';
            length--;
        }
        assert(partsIndex < lineCount);

        const char *codeOriginal = str_trim_in_place(&block[stringStartIndex], length, &length);
        parts[partsIndex].code = codeOriginal;
        parts[partsIndex].codeLength = length;
        parts[partsIndex].index = partsIndex;

        partsAsc[partsIndex].code = str_to_upper(codeOriginal, length, &block[blockIndexUpper]);
        partsAsc[partsIndex].codeLength = length;
        partsAsc[partsIndex].index = partsIndex;

        blockIndexUpper += length + 1; // +1 for null terminator
        stringStartIndex = i + 1;
        partsIndex++;
    }

    qsort(partsAsc, partsIndex, sizeof(*partsAsc), compare_part_by_code_length_asc);

    data->partsOriginal = parts;
    data->partsOriginalCount = partsIndex;
    data->partsAsc = partsAsc;
    data->partsAscCount = partsIndex;
}

static void build_masterParts(const char *masterPartsPath, SourceData *data) {
    size_t lineCount;
    size_t contentSize;
    char *block = read_file(masterPartsPath, 2, &contentSize, &lineCount);

    MasterPart *masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);
    MasterPart *masterPartsAsc = malloc(lineCount * sizeof(*masterPartsAsc));
    CHECK_ALLOC(masterPartsAsc);
    MasterPart *masterPartsAscNh = malloc(lineCount * sizeof(*masterPartsAscNh));
    CHECK_ALLOC(masterPartsAscNh);

    size_t mpIndex = 0;
    size_t mpNohIndex = 0;
    size_t stringStartIndex = 0;
    size_t blockIndexExtra = contentSize;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] != '\n') {
            continue;
        }
        block[i] = '\0';
        size_t length = i - stringStartIndex;
        if (i > 0 && block[i - 1] == '\r') {
            block[i - 1] = '\0';
            length--;
        }
        assert(mpIndex < lineCount);

        const char *codeOriginal = str_trim_in_place(&block[stringStartIndex], length, &length);
        if (length >= MIN_STRING_LENGTH) {
            masterParts[mpIndex].code = codeOriginal;
            masterParts[mpIndex].codeLength = length;
            masterParts[mpIndex].index = mpIndex;

            const char *codeUpper = str_to_upper(codeOriginal, length, &block[blockIndexExtra]);
            masterPartsAsc[mpIndex].code = codeUpper;
            masterPartsAsc[mpIndex].codeLength = length;
            masterPartsAsc[mpIndex].index = mpIndex;
            blockIndexExtra += length + 1; // +1 for null terminator

            if (str_contains_dash(codeUpper, length)) {
                size_t codeNhLength;
                masterPartsAscNh[mpNohIndex].code = str_remove_char(codeUpper, length, &block[blockIndexExtra], '-', &codeNhLength);
                masterPartsAscNh[mpNohIndex].codeLength = codeNhLength;
                masterPartsAscNh[mpNohIndex].index = mpIndex;
                mpNohIndex++;
                blockIndexExtra += codeNhLength + 1; // +1 for null terminator
            }

            mpIndex++;
        }
        stringStartIndex = i + 1;
    }

    qsort(masterPartsAsc, mpIndex, sizeof(*masterPartsAsc), compare_mp_by_code_length_asc);
    qsort(masterPartsAscNh, mpNohIndex, sizeof(*masterPartsAscNh), compare_mp_by_code_length_asc);

    data->masterPartsOriginal = masterParts;
    data->masterPartsOriginalCount = mpIndex;
    data->masterPartsAsc = masterPartsAsc;
    data->masterPartsAscCount = mpIndex;
    data->masterPartsAscNh = masterPartsAscNh;
    data->masterPartsAscNhCount = mpNohIndex;
}

static int compare_mp_by_code_length_asc(const void *a, const void *b) {
    const MasterPart *mpA = (const MasterPart *)a;
    const MasterPart *mpB = (const MasterPart *)b;
    return (mpA->codeLength < mpB->codeLength) ? -1
        : (mpA->codeLength > mpB->codeLength) ? 1
        : (mpA->index < mpB->index) ? -1 : (mpA->index > mpB->index) ? 1 : 0;
}
static int compare_part_by_code_length_asc(const void *a, const void *b) {
    const Part *pA = (const Part *)a;
    const Part *pB = (const Part *)b;
    return (pA->codeLength < pB->codeLength) ? -1
        : (pA->codeLength > pB->codeLength) ? 1
        : (pA->index < pB->index) ? -1 : (pA->index > pB->index) ? 1 : 0;
}


/*
* By applying simple qsort, the whole app is 50% faster.
* But, it's not a stable sort. It still finds the same number of matches though.
* However, in case of ties, it might return any of them.
* Requirements specify for ties to return the first masterParts in the file.
*/
//static int compare_mp_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const MasterPart *)a)->codeLength;
//    size_t lenB = ((const MasterPart *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
//
//static int compare_part_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const Part *)a)->codeLength;
//    size_t lenB = ((const Part *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
