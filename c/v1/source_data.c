#include <ctype.h>
#include <sys/stat.h>
#include "common.h"
#include "source_data.h"

static Part *build_parts(const char *partsPath, size_t *outCount);
static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount);
static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut);
const char *str_trim_in_place(char *src, size_t srcLength, size_t *outLength);
const char *str_to_upper(const char *src, size_t srcLength, char *buffer);
const char *str_remove_char(const char *src, size_t srcLength, char *buffer, char find, size_t *outLength);
bool str_contains_dash(const char *str, size_t strLength);

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
    data->blockMasterPartCodes = masterParts->codeOriginal;
    data->blockPartCodes = parts->codeOriginal;

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
    size_t lineCount;
    size_t contentSize;
    char *block = read_file(partsPath, 2, &contentSize, &lineCount);
    Part *parts = malloc(lineCount * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t stringStartIndex = 0;
    size_t partsIndex = 0;
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
        parts[partsIndex].codeOriginal = codeOriginal;
        parts[partsIndex].codeLength = length;
        parts[partsIndex].code = str_to_upper(codeOriginal, length, &block[blockIndexUpper]);
        parts[partsIndex].index = partsIndex;
        blockIndexUpper += length + 1; // +1 for null terminator
        stringStartIndex = i + 1;
        partsIndex++;
    }

    *outCount = partsIndex;
    return parts;
}

static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount) {
    size_t lineCount;
    size_t contentSize;
    char *block = read_file(masterPartsPath, 2, &contentSize, &lineCount);
    MasterPart *masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);

    size_t stringStartIndex = 0;
    size_t masterPartsIndex = 0;
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
        assert(masterPartsIndex < lineCount);

        const char *codeOriginal = str_trim_in_place(&block[stringStartIndex], length, &length);
        if (length >= MIN_STRING_LENGTH) {
            masterParts[masterPartsIndex].codeOriginal = codeOriginal;
            masterParts[masterPartsIndex].codeLength = length;
            masterParts[masterPartsIndex].index = masterPartsIndex;

            const char *code = str_to_upper(codeOriginal, length, &block[blockIndexExtra]);
            masterParts[masterPartsIndex].code = code;
            blockIndexExtra += length + 1; // +1 for null terminator

            if (str_contains_dash(code, length)) {
                size_t codeNhLength;
                masterParts[masterPartsIndex].codeNh = str_remove_char(code, length, &block[blockIndexExtra], '-', &codeNhLength);
                masterParts[masterPartsIndex].codeNhLength = codeNhLength;
                blockIndexExtra += codeNhLength + 1; // +1 for null terminator
            }
            else {
                masterParts[masterPartsIndex].codeNh = NULL;
                masterParts[masterPartsIndex].codeNhLength = 0;
            }
            masterPartsIndex++;
        }
        stringStartIndex = i + 1;
    }

    *outCount = masterPartsIndex;
    return masterParts;
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
    // It thinks there is a buffer overrun, it can't calculate based on sizeFactor
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

bool str_contains_dash(const char *str, size_t strLength) {
    assert(str);

    for (size_t i = 0; i < strLength; i++) {
        if (str[i] == '-') {
            return true;
        }
    }
    return false;
}

const char *str_to_upper(const char *src, size_t srcLength, char *buffer) {
    assert(src);
    assert(buffer);

    // We know the buffer has enough space.
    for (size_t i = 0; i < srcLength; i++) {
        buffer[i] = toupper(src[i]);
    }
    buffer[srcLength] = '\0';
    return buffer;
}

const char *str_remove_char(const char *src, size_t srcLength, char *buffer, char find, size_t *outLength) {
    assert(src);
    assert(buffer);
    assert(outLength);

    // We know the buffer has enough space.
    size_t j = 0;
    for (size_t i = 0; i < srcLength; i++) {
        if (src[i] != find) {
            // We know the chars are ASCII (no need to cast to unaligned char)
            buffer[j++] = toupper(src[i]);
        }
    }
    buffer[j] = '\0';
    *outLength = j;
    return buffer;
}

const char *str_trim_in_place(char *src, size_t srcLength, size_t *outLength) {
    assert(src);
    assert(outLength);

    size_t start = 0;
    while (start < srcLength && isspace(src[start])) {
        start++;
    }

    if (start == srcLength) {
        src[0] = '\0';
        *outLength = 0;
        return src;
    }

    size_t end = srcLength - 1;
    while (end > start && isspace(src[end])) {
        end--;
    }

    src[end + 1] = '\0';
    *outLength = end - start + 1;
    return &src[start];
}
