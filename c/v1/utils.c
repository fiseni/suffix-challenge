#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <immintrin.h>
#include <sys/stat.h>
#include "utils.h"

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

bool str_equals_same_length(const char *s1, const char *s2, size_t length) {
    assert(s1);
    assert(s2);

    for (size_t i = 0; i < length; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

bool str_equals_same_length_vectorized(const char *s1, const char *s2, size_t length) {
    assert(s1);
    assert(s2);

    size_t i = 0;

    // Process 32 bytes at a time using AVX2
    // There are very few cases where strings are longer than 32 bytes. Not worth it.
    /*
    for (; i + 31 < length; i += 32) {
        // Load 32 bytes from each string (unaligned load)
        __m256i chunk1 = _mm256_loadu_si256((const __m256i*)(s1 + i));
        __m256i chunk2 = _mm256_loadu_si256((const __m256i*)(s2 + i));

        __m256i cmp = _mm256_cmpeq_epi8(chunk1, chunk2);
        int mask = _mm256_movemask_epi8(cmp);

        if (mask != 0xFFFFFFFF) {
            return false;
        }
    }
    */

    // Process 16 bytes at a time using SSE2
    for (; i + 15 < length; i += 16) {
        // Load 16 bytes from each string (unaligned load)
        __m128i chunk1 = _mm_loadu_si128((const __m128i *)(s1 + i));
        __m128i chunk2 = _mm_loadu_si128((const __m128i *)(s2 + i));

        __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
        int mask = _mm_movemask_epi8(cmp);

        if (mask != 0xFFFF) {
            return false;
        }
    }

    // Compare any remaining bytes
    for (; i < length; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }

    return true;
}

bool str_is_suffix(const char *value, size_t valueLength, const char *source, size_t sourceLength) {
    assert(value);
    assert(source);

    if (valueLength > sourceLength) {
        return false;
    }
    const char *endOfSource = source + (sourceLength - valueLength);
    //return (strcmp(endOfSource, value) == 0);
    return (memcmp(endOfSource, value, valueLength) == 0);
}

bool str_is_suffix_vectorized(const char *value, size_t valueLength, const char *source, size_t sourceLength) {
    assert(value);
    assert(source);

    if (valueLength > sourceLength) {
        return false;
    }
    const char *endOfSource = source + (sourceLength - valueLength);

    // For our use-case most of the time the strings are not equal.
    // It turns out checking the first char before vectorization improves the performance.
    return (endOfSource[0] == value[0] && str_equals_same_length_vectorized(endOfSource, value, valueLength));
}

void str_to_upper_trim(const char *src, char *buffer, size_t bufferSize, size_t *outBufferLength) {
    assert(src);
    assert(buffer);
    assert(outBufferLength);
    assert(bufferSize > 0);

    // We know the chars are ASCII (no need to cast to unaligned char)

    size_t srcLength = strlen(src);
    size_t start = 0;
    while (start < srcLength && isspace(src[start])) {
        start++;
    }

    if (start == srcLength) {
        buffer[0] = '\0';
        *outBufferLength = 0;
        return;
    }

    size_t end = srcLength - 1;
    while (end > start && isspace(src[end])) {
        end--;
    }

    size_t j = 0;
    for (size_t i = start; i <= end && j < bufferSize - 1; i++) {
        buffer[j++] = toupper(src[i]);
    }
    buffer[j] = '\0';
    *outBufferLength = j;
}

void str_to_upper_trim_in_place(char *src, size_t length, size_t *outLength) {
    assert(src);
    assert(outLength);

    // We know the chars are ASCII (no need to cast to unaligned char)

    size_t start = 0;
    while (start < length && isspace(src[start])) {
        start++;
    }

    if (start == length) {
        src[0] = '\0';
        *outLength = 0;
        return;
    }

    size_t end = length - 1;
    while (end > start && isspace(src[end])) {
        end--;
    }

    size_t j = 0;
    for (size_t i = start; i <= end; i++) {
        src[j++] = toupper(src[i]);
    }
    src[j] = '\0';
    *outLength = j;
}

void str_trim(const char *src, char *buffer, size_t bufferSize, size_t *outBufferLength) {
    assert(src);
    assert(buffer);
    assert(outBufferLength);
    assert(bufferSize > 0);

    // We know the chars are ASCII (no need to cast to unaligned char)

    size_t srcLength = strlen(src);
    size_t start = 0;
    while (start < srcLength && isspace(src[start])) {
        start++;
    }

    if (start == srcLength) {
        buffer[0] = '\0';
        *outBufferLength = 0;
        return;
    }

    size_t end = srcLength - 1;
    while (end > start && isspace(src[end])) {
        end--;
    }

    size_t j = 0;
    for (size_t i = start; i <= end && j < bufferSize - 1; i++) {
        buffer[j++] = src[i];
    }
    buffer[j] = '\0';
    *outBufferLength = j;
}

void str_trim_in_place(char *src, size_t *outLength) {
    assert(src);
    assert(outLength);

    // We know the chars are ASCII (no need to cast to unaligned char)

    size_t length = strlen(src);
    size_t start = 0;
    while (start < length && isspace(src[start])) {
        start++;
    }

    if (start == length) {
        src[0] = '\0';
        *outLength = 0;
        return;
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

bool is_power_of_two(size_t n) {
    if (n == 0)
        return false;
    return (n & (n - 1)) == 0;
}

size_t next_power_of_two(size_t n) {
    if (n == 0)
        return 1;
    if (is_power_of_two(n))
        return n;

    // Subtract 1 to ensure correct bit setting for the next power of 2
    n--;
    int bits = sizeof(size_t) * CHAR_BIT;

    // Set all bits to the right of the MSB
    for (int shift = 1; shift < bits; shift <<= 1) {
        n |= n >> shift;
    }

    // Add 1 to get the next power of 2
    n++;

    // For clarity in case of overflow.
    // If n becomes 0 after shifting, it means the next power of 2 exceeds the limit
    if (n == 0)
        return 0;
    return n;
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
