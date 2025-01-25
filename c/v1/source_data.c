#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>
#include "common.h"
#include "source_data.h"

static Part *build_parts(const char *partsPath, size_t *outCount);
static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount);
static bool populate_masterPart(MasterPart *masterPart, size_t masterPartsIndex, char *code, size_t codeLength, char *block, size_t blockSize, size_t *blockIndexNoHyphens);
bool str_contains_dash(const char *str, size_t strLength);
const char *str_trim_in_place(char *src, size_t *outLength);
void str_remove_char(const char *src, size_t srcLength, char *buffer, size_t bufferSize, char find, size_t *outBufferLength);
long get_file_size_bytes(const char *filename);

const SourceData *source_data_read(const char *masterPartsPath, const char *partsPath) {
    size_t masterPartsCount = 0;
    MasterPart *masterParts = build_masterParts(masterPartsPath, &masterPartsCount);
    size_t partsCount = 0;
    Part *parts = build_parts(partsPath, &partsCount);

    SourceData *data = (SourceData *)malloc(sizeof(*data));
    CHECK_ALLOC(data);
    data->masterParts = masterParts;
    data->masterPartsCount = masterPartsCount;
    data->parts = parts;
    data->partsCount = partsCount;
    data->blockMasterPartCodes = masterParts->code;
    data->blockPartCodes = parts->code;

    return data;
}

void source_data_clean(const SourceData *data) {
    // All strings are allocated from a single block
    free((void *)data->blockMasterPartCodes);
    free((void *)data->blockPartCodes);

    free((void *)data->masterParts);
    free((void *)data->parts);
    free((void *)data);
}

static Part *build_parts(const char *partsPath, size_t *outCount) {
    long fileSize = get_file_size_bytes(partsPath);
    FILE *file = fopen(partsPath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open parts file: %s\n", partsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * fileSize + 1;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

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
    Part *parts = malloc(lineCount * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t stringStartIndex = 0;
    size_t partsIndex = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            size_t length = i - stringStartIndex;
            if (i > 0 && block[i - 1] == '\r') {
                block[i - 1] = '\0';
                length--;
            }
            assert(partsIndex < lineCount);
            parts[partsIndex].code = str_trim_in_place(&block[stringStartIndex], &length);
            parts[partsIndex].codeLength = length;
            parts[partsIndex].index = partsIndex;
            partsIndex++;
            stringStartIndex = i + 1;
        }
    }

    *outCount = partsIndex;
    return parts;
}

static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount) {
    long fileSize = get_file_size_bytes(masterPartsPath);
    FILE *file = fopen(masterPartsPath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open file: %s\n", masterPartsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * fileSize * 2 + 2;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

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
    MasterPart *masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);

    size_t stringStartIndex = 0;
    size_t masterPartsIndex = 0;
    size_t blockIndexNoHyphens = contentSize;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            size_t length = i - stringStartIndex;
            if (i > 0 && block[i - 1] == '\r') {
                block[i - 1] = '\0';
                length--;
            }
            assert(masterPartsIndex < lineCount);
            if (populate_masterPart(&masterParts[masterPartsIndex], masterPartsIndex, &block[stringStartIndex], length, block, blockSize, &blockIndexNoHyphens)) {
                masterPartsIndex++;
            }
            stringStartIndex = i + 1;
        }
    }

    *outCount = masterPartsIndex;
    return masterParts;
}

static bool populate_masterPart(MasterPart *masterPart, size_t masterPartsIndex, char *code, size_t codeLength, char *block, size_t blockSize, size_t *blockIndexNoHyphens) {
    if (codeLength < MIN_STRING_LENGTH) {
        return false;
    }

    code = (char *)str_trim_in_place(code, &codeLength);
    if (codeLength < MIN_STRING_LENGTH) {
        return false;
    }

    masterPart->code = code;
    masterPart->codeLength = codeLength;
    masterPart->index = masterPartsIndex;

    if (str_contains_dash(code, codeLength)) {
        // Ensure there is enough space in the block for codeNh
        if (*blockIndexNoHyphens + codeLength + 1 > blockSize) { // +1 for null terminator
            fprintf(stderr, "Block overflow detected while allocating codeNh.\n");
            exit(EXIT_FAILURE);
        }

        char *codeNh = &block[*blockIndexNoHyphens];
        size_t codeNhLength;
        str_remove_char(code, codeLength, codeNh, codeLength, '-', &codeNhLength);
        masterPart->codeNh = codeNh;
        masterPart->codeNhLength = codeNhLength;

        *blockIndexNoHyphens += codeNhLength + 1; // +1 for null terminator
    }
    else {
        masterPart->codeNh = NULL;
        masterPart->codeNhLength = 0;
    }
    return true;
}

long get_file_size_bytes(const char *filename) {
    assert(filename);

    struct stat st;
    if (stat(filename, &st) != 0) {
        perror("stat failed");
        return -1;
    }
    return st.st_size;
}

bool str_contains_dash(const char *str, size_t strLength) {
    assert(str);

    for (size_t i = 0; i < strLength; i++) {
        if (str[i] == '-') {
            return true;
        }
    }
    return false;
}

const char *str_trim_in_place(char *src, size_t *outLength) {
    assert(src);
    assert(outLength);

    size_t length = *outLength;
    size_t start = 0;
    while (start < length && isspace(src[start])) {
        start++;
    }

    if (start == length) {
        src[0] = '\0';
        *outLength = 0;
        return src;
    }

    size_t end = length - 1;
    while (end > start && isspace(src[end])) {
        end--;
    }

    size_t j = 0;
    for (size_t i = start; i <= end; i++) {
        src[j++] = src[i];
    }
    src[j] = '\0';
    *outLength = j;
    return src;
}

void str_remove_char(const char *src, size_t srcLength, char *buffer, size_t bufferSize, char find, size_t *outBufferLength) {
    assert(src);
    assert(buffer);
    assert(outBufferLength);

    if (bufferSize == 0) {
        *outBufferLength = 0;
        return;
    }
    size_t j = 0;
    for (size_t i = 0; i < srcLength && j < bufferSize - 1; i++) {
        if (src[i] != find) {
            // We know the chars are ASCII (no need to cast to unaligned char)
            buffer[j++] = toupper(src[i]);
        }
    }
    buffer[j] = '\0';
    *outBufferLength = j;
}
