#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)


static const size_t MAX_SIZE_T_VALUE = ((size_t)-1);

const char *str_trim_in_place(char *src, size_t srcLength, size_t * outLength);
const char *str_to_upper(const char *src, size_t srcLength, char *buffer);
const char *str_remove_char(const char *src, size_t srcLength, char *buffer, char find, size_t * outLength);
bool str_contains_dash(const char *str, size_t strLength);
char *read_file(const char *filePath, unsigned int sizeFactor, size_t * contentSizeOut, size_t * lineCountOut);

#endif
