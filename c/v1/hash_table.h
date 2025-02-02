#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>

// #########################################################
// Hash table storing a size_t.
typedef struct Entry {
    const char *key;
    size_t value;
    struct Entry *next;
} Entry;

typedef struct HTable {
    Entry **buckets;
    size_t size;

    Entry *blockEntries;
    size_t blockEntriesCount;
    size_t blockEntriesIndex;
} HTable;

HTable *htable_create(size_t size);
bool htable_search(const HTable *table, const char *key, size_t keyLength, size_t *outValue);
void htable_insert_if_not_exists(HTable *table, const char *key, size_t keyLength, size_t value);
void htable_free(HTable *table);

// #########################################################

static inline size_t hash(size_t tableSize, const char *key, size_t keyLength) {
    size_t hash = 0x811C9DC5; // 2166136261
    for (size_t i = 0; i < keyLength; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash & (tableSize - 1);
}

static inline size_t next_power_of_two(size_t n) {
    if (n == 0)
        return 1;

    // If n is already a power of 2, return n
    if ((n & (n - 1)) == 0)
        return n;

    // Subtract 1 to ensure correct bit setting for the next power of 2
    n--;
    int bits = sizeof(size_t) * 8;

    // Set all bits to the right of the MSB
    for (int shift = 1; shift < bits; shift <<= 1) {
        n |= n >> shift;
    }

    // Add 1 to get the next power of 2
    n++;

    // For clarity, in case of overflow.
    // If n becomes 0 after shifting, it means the next power of 2 exceeds the limit
    if (n == 0)
        return 0;

    return n;
}

#endif
