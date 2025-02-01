#include <sys/stat.h>
#include "common.h"
#include "source_data.h"

static void build_parts(const char *partsPath, SourceData *data);
static void build_masterParts(const char *masterPartsPath, SourceData *data);
static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut);
static int compare_by_code_length_asc(const void *a, const void *b);

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

    qsort(partsAsc, partsIndex, sizeof(*partsAsc), compare_by_code_length_asc);

    data->partsOriginal = parts;
    data->partsOriginalCount = partsIndex;
    data->partsAsc = partsAsc;
    data->partsAscCount = partsIndex;
}

static void build_masterParts(const char *masterPartsPath, SourceData *data) {
    size_t lineCount;
    size_t contentSize;
    char *block = read_file(masterPartsPath, 2, &contentSize, &lineCount);

    Part *masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);
    Part *masterPartsAsc = malloc(lineCount * sizeof(*masterPartsAsc));
    CHECK_ALLOC(masterPartsAsc);
    Part *masterPartsAscNh = malloc(lineCount * sizeof(*masterPartsAscNh));
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

    qsort(masterPartsAsc, mpIndex, sizeof(*masterPartsAsc), compare_by_code_length_asc);
    qsort(masterPartsAscNh, mpNohIndex, sizeof(*masterPartsAscNh), compare_by_code_length_asc);

    data->masterPartsOriginal = masterParts;
    data->masterPartsOriginalCount = mpIndex;
    data->masterPartsAsc = masterPartsAsc;
    data->masterPartsAscCount = mpIndex;
    data->masterPartsAscNh = masterPartsAscNh;
    data->masterPartsAscNhCount = mpNohIndex;
}

static long get_file_size_bytes(const char *filePath) {
    assert(filePath);

    struct stat st;
    if (stat(filePath, &st) != 0) {
        perror("stat failed");
        return -1;
    }
    return st.st_size;
}

static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut) {
    long fileSize = get_file_size_bytes(filePath);
    FILE *file = fopen(filePath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open file: %s\n", filePath);
        exit(EXIT_FAILURE);
    }
    assert(fileSize > 0);
    assert(sizeFactor > 0);

    size_t blockSize = sizeof(char) * fileSize * sizeFactor + sizeFactor;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

#pragma warning(disable: 6385 6386 )
    // MSVC thinks there is a buffer overrun, it can't calculate based on sizeFactor.
    // If you put a literal for sizeFactor, e.g. 1, warning goes way.

    // Handle the case where the file does not end with a newline
    if (contentSize > 0 && block[contentSize - 1] != '\n') {
        block[contentSize] = '\n';
        contentSize++;
    }

    size_t lineCount = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            lineCount++;
        }
    }
#pragma warning(default: 6385 6386  )

    *contentSizeOut = contentSize;
    *lineCountOut = lineCount;
    return block;
}

static int compare_by_code_length_asc(const void *a, const void *b) {
    const Part *partA = (const Part *)a;
    const Part *partB = (const Part *)b;
    return (partA->codeLength < partB->codeLength) ? -1
        : (partA->codeLength > partB->codeLength) ? 1
        : (partA->index < partB->index) ? -1 : (partA->index > partB->index) ? 1 : 0;
}

/*
* By applying simple qsort, the whole app is 30% faster (270ms wall time down from 400ms).
* But, it's not a stable sort. It still finds the same number of matches though.
* However, in case of ties, it might return any of them.
* Requirements specify that for ties we should return the first masterParts in the file.
*/
//static int compare_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const Part *)a)->codeLength;
//    size_t lenB = ((const Part *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
