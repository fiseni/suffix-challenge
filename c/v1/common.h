#ifndef COMMON_H
#define COMMON_H

/* Fati Iseni
* DO NOT use these implementations for general purpose stuff.
* They're tailored for our scenario and they're safe to use only within this context.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define CHAR_SPACE ' '
#define CHAR_HYPHEN '-'
#define CHAR_SEMICOLON ';'

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)

static const size_t MAX_SIZE_T_VALUE = ((size_t)-1);

static inline const char *str_to_upper(const char *src, size_t srcLength, char *buffer) {
    assert(src);
    assert(buffer);

    // We know the buffer has enough space.
    for (size_t i = 0; i < srcLength; i++) {
        buffer[i] = (unsigned int)(src[i] - 97) <= 25 // 25 = 122 - 97
            ? src[i] & 0x5F
            : src[i];
    }
    buffer[srcLength] = '\0';
    return buffer;
}

static inline const char *str_remove_hyphens(const char *src, size_t srcLength, char *buffer, size_t *outLength) {
    assert(src);
    assert(buffer);
    assert(outLength);

    // We know the buffer has enough space.
    size_t j = 0;
    for (size_t i = 0; i < srcLength; i++) {
        if (src[i] != CHAR_HYPHEN) {
            // We know the chars are ASCII (no need to cast to unaligned char)
            buffer[j++] = src[i];
        }
    }
    buffer[j] = '\0';
    *outLength = j;
    return buffer;
}

static inline const char *str_trim_in_place(const char *src, size_t srcLength, size_t *outLength) {
    assert(src);
    assert(outLength);

    size_t start = 0;
    while (start < srcLength && src[start] == CHAR_SPACE) {
        start++;
    }

    if (start == srcLength) {
        *outLength = 0;
        return src;
    }

    size_t end = srcLength - 1;
    while (end > start && src[end] == CHAR_SPACE) {
        end--;
    }

    // We won't write null termination characters. The src is memory mapped file.
    *outLength = end - start + 1;
    return &src[start];
}

#endif
