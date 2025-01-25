#include "cross_platform_time.h"

#if defined(_WIN32) || defined(_WIN64)  // Windows

#include <windows.h>

double time_get_seconds(void) {
    static LARGE_INTEGER frequency;
    static int initialized = 0;

    // One-time initialization of frequency
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    // Return seconds as a double
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

#else  // POSIX (Linux, macOS, etc.)

//#define _POSIX_C_SOURCE 199309L
#include <sys/time.h>
#include <stddef.h>

//double time_get_seconds(void) {
//    struct timespec now;
//    clock_gettime(CLOCK_MONOTONIC, &now);
//    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
//}

double time_get_seconds(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
}

#endif
