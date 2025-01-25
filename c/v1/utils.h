#ifndef COMMON_H
#define COMMON_H

#ifdef _MSC_VER
#define strdup _strdup
#endif

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#else // assuming POSIX or BSD compliant system
#include <strings.h>
#endif

#include <stdlib.h>
#include <stdbool.h>

bool str_contains_dash(const char *str, size_t strLength);
bool str_equals_same_length(const char *s1, const char *s2, size_t length);
bool str_equals_same_length_vectorized(const char *s1, const char *s2, size_t length);
bool str_is_suffix(const char *value, size_t valueLength, const char *source, size_t sourceLength);
bool str_is_suffix_vectorized(const char *value, size_t valueLength, const char *source, size_t sourceLength);
void str_to_upper_trim(const char *src, char *buffer, size_t bufferSize, size_t * outBufferLength);
void str_to_upper_trim_in_place(char *src, size_t length, size_t * outLength);
void str_trim(const char *src, char *buffer, size_t bufferSize, size_t * outBufferLength);
void str_trim_in_place(char *src, size_t * outLength);
void str_remove_char(const char *src, size_t srcLength, char *buffer, size_t bufferSize, char find, size_t * outBufferLength);

bool is_power_of_two(size_t n);
size_t next_power_of_two(size_t n);
long get_file_size_bytes(const char *filename);

#endif
