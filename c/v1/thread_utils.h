#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#define CHECK_THREAD_CREATE_STATUS(status, length)                              \
    do {                                                                        \
        if (status != 0) {                                                      \
            fprintf(stderr, "Error creating thread for length %zu\n", length);  \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

#define CHECK_THREAD_JOIN_STATUS(status, length)                                \
    do {                                                                        \
        if (status != 0) {                                                      \
            fprintf(stderr, "Error joining thread for length %zu\n", length);   \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)


#if defined(_WIN32) || defined(_WIN64)  // Windows
// Windows-specific includes and definitions
#include <windows.h>
typedef HANDLE thread_t;
typedef DWORD thread_ret_t;
typedef LPVOID thread_arg_t;
#else
// POSIX-specific includes and definitions
#include <pthread.h>
typedef pthread_t thread_t;
typedef void *thread_ret_t;
typedef void *thread_arg_t;
#endif

// Thread function signature
typedef thread_ret_t(*thread_func_t)(thread_arg_t);

// Thread creation function
int create_thread(thread_t *thread, thread_func_t func, thread_arg_t arg);

// Thread join function
int join_thread(thread_t thread, thread_ret_t *ret);

#endif
