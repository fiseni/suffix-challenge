#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "allocator.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose allocator.
* It is tailored for our scenario, bunch of checks are omitted and it is safe to use only within this apps' context.
*/

static const size_t BLOCK_SIZE_INITIAL = (size_t)(1000 * 1024) * 1024;
static const size_t ALIGNMENT = 64;

static unsigned char *block = NULL;
static size_t blockSize = 0;
static size_t offset = 0;
static thread_mutex_t mutex;

// This will be called at startup on the main thread.
void allocator_init() {

    thread_mutex_init(&mutex);

    blockSize = BLOCK_SIZE_INITIAL;
    block = (unsigned char *)malloc(blockSize);
    offset = 0;

    if (block == NULL) {
        thread_mutex_destroy(&mutex);
        fprintf(stderr, "Failed to initialize the allocator!\n");
        exit(EXIT_FAILURE);
    }
}

void *allocator_alloc(size_t size) {
    thread_mutex_lock(&mutex);

    // Alignment padding
    uintptr_t currentAddress = (uintptr_t)(block + offset);
    size_t padding = (ALIGNMENT - (currentAddress % ALIGNMENT)) % ALIGNMENT;

    if (offset + padding + size > blockSize) {
        thread_mutex_unlock(&mutex);
        fprintf(stderr, "Not enough space in the allocator!\n");
        return NULL;
    }

    offset += padding;
    void *ptr = block + offset;
    offset += size;

    thread_mutex_unlock(&mutex);
    return ptr;
}

void allocator_destroy() {
    thread_mutex_lock(&mutex);

    free(block);
    block = NULL;
    blockSize = 0;
    offset = 0;

    thread_mutex_unlock(&mutex);
    thread_mutex_destroy(&mutex);
}
