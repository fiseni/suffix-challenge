// thread_utils.c
#include "thread_utils.h"

#ifdef _WIN32

// Wrapper for Windows thread function
typedef struct {
    thread_func_t func;
    thread_arg_t arg;
} thread_wrapper_t;

// Windows thread procedure
static DWORD WINAPI thread_proc(LPVOID param) {
    thread_wrapper_t *wrapper = (thread_wrapper_t *)param;
    wrapper->func(wrapper->arg);
    free(wrapper);
    return 0;
}

int create_thread(thread_t *thread, thread_func_t func, thread_arg_t arg) {
    thread_wrapper_t *wrapper = malloc(sizeof(thread_wrapper_t));
    if (!wrapper) return -1;
    wrapper->func = func;
    wrapper->arg = arg;
    *thread = CreateThread(
        NULL,               // default security attributes
        0,                  // default stack size
        thread_proc,        // thread function
        wrapper,            // parameter to thread function
        0,                  // default creation flags
        NULL                // receive thread identifier
    );
    if (*thread == NULL) {
        free(wrapper);
        return -1;
    }
    return 0;
}

int join_thread(thread_t thread, thread_ret_t *ret) {
    WaitForSingleObject(thread, INFINITE);
    if (ret) {
        DWORD exit_code;
        if (GetExitCodeThread(thread, &exit_code)) {
            *ret = (thread_ret_t)exit_code;
        }
    }
    CloseHandle(thread);
    return 0;
}

#else

int create_thread(thread_t *thread, thread_func_t func, thread_arg_t arg) {
    return pthread_create(thread, NULL, func, arg);
}

int join_thread(thread_t thread, thread_ret_t *ret) {
    return pthread_join(thread, ret);
}

#endif
