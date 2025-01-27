#include <ctype.h>
#include <sys/stat.h>
#include "common.h"

/* Fati Iseni
* DO NOT use these implementations for general purpose stuff.
* They're tailored for our scenario and they're safe to use only within this context.
*/

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

static long get_file_size_bytes(const char *filePath) {
    assert(filePath);

    struct stat st;
    if (stat(filePath, &st) != 0) {
        perror("stat failed");
        return -1;
    }
    return st.st_size;
}

char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut) {
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
