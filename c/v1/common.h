#ifndef COMMON_H
#define COMMON_H

/* Fati Iseni
* DO NOT use these implementations for general purpose stuff.
* They're tailored for our scenario and they're safe to use only within this context.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)


static const size_t MAX_SIZE_T_VALUE = ((size_t)-1);

static inline bool str_contains_dash(const char *str, size_t strLength) {
    assert(str);

    for (size_t i = 0; i < strLength; i++) {
        if (str[i] == '-') {
            return true;
        }
    }
    return false;
}

static inline const char *str_to_upper(const char *src, size_t srcLength, char *buffer) {
    assert(src);
    assert(buffer);

    // We know the buffer has enough space.
    for (size_t i = 0; i < srcLength; i++) {
        buffer[i] = toupper(src[i]);
    }
    buffer[srcLength] = '\0';
    return buffer;
}

static inline const char *str_remove_char(const char *src, size_t srcLength, char *buffer, char find, size_t * outLength) {
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

static inline const char *str_trim_in_place(char *src, size_t srcLength, size_t * outLength) {
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

#endif
