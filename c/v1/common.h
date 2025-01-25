#ifndef COMMON_H
#define COMMON_H

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)


#if defined(_WIN32) || defined(_WIN64)
// Windows includes and definitions
#include <string.h>
#define strcasecmp _stricmp
#else
// POSIX-specific includes and definitions
#include <strings.h>
#endif

#endif
