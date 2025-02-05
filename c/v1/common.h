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

typedef struct StringView {
    const char *data;
    size_t length;
} StringView;

static const size_t MAX_SIZE_T_VALUE = ((size_t)-1);

static inline StringView sv_to_upper(const StringView *src, char *dest) {
    assert(src);
    assert(dest);

    // We know the dest has enough space.
    for (size_t i = 0; i < src->length; i++) {
        dest[i] = (unsigned int)(src->data[i] - 97) <= 25 // 25 = 122 - 97
            ? src->data[i] & 0x5F
            : src->data[i];
    }

    return (StringView) {
        .data = dest,
        .length = src->length
    };
}

static inline StringView sv_to_no_hyphens(StringView *src, char *dest) {
    assert(src);
    assert(dest);

    // We know the dest has enough space.
    size_t j = 0;
    for (size_t i = 0; i < src->length; i++) {
        if (src->data[i] != CHAR_HYPHEN) {
            // We know the chars are ASCII (no need to cast to unaligned char)
            dest[j++] = src->data[i];
        }
    }

    return (StringView) {
        .data = dest,
        .length = j
    };
}

static inline StringView sv_slice_suffix(const StringView *src, size_t suffixLength) {
    return (StringView) {
        .data = src->data + (src->length - suffixLength),
        .length = suffixLength
    };
}

static inline StringView sv_slice_trim(const char *src, size_t length) {
    assert(src);

    size_t start = 0;
    while (start < length && src[start] == CHAR_SPACE) {
        start++;
    }

    if (start == length) {
        return (StringView) {
            .data = src,
            .length = 0
        };
    }

    size_t end = length - 1;
    while (end > start && src[end] == CHAR_SPACE) {
        end--;
    }

    return (StringView) {
        .data = &src[start],
        .length = end - start + 1
    };
}

#endif
