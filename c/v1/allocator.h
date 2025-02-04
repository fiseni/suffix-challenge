#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#if defined(_WIN32) || defined(_WIN64)
// Windows-specific includes and definitions
#include <windows.h>

typedef CRITICAL_SECTION thread_mutex_t;

// Modify the macros to handle initialization failures
#define thread_mutex_init(mutex) InitializeCriticalSection(mutex)
#define thread_mutex_lock(mutex) EnterCriticalSection(mutex)
#define thread_mutex_unlock(mutex) LeaveCriticalSection(mutex)
#define thread_mutex_destroy(mutex) DeleteCriticalSection(mutex)

#else
// POSIX-specific includes and definitions
#include <pthread.h>

typedef pthread_mutex_t thread_mutex_t;

#define thread_mutex_init(mutex) pthread_mutex_init(mutex, NULL)
#define thread_mutex_lock(mutex) pthread_mutex_lock(mutex)
#define thread_mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#define thread_mutex_destroy(mutex) pthread_mutex_destroy(mutex)

#endif

#include <stdlib.h>

void allocator_init();
void allocator_destroy();
void *allocator_alloc(size_t size);

#endif
