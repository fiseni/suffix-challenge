#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <ctype.h>
#include <limits.h>

// #########################################################
// Hash table storing a string.
typedef struct EntryString {
    const char *key;
    const char *value;
    struct EntryString *next;
} EntryString;

typedef struct HTableString {
    EntryString **buckets;
    size_t size;
    EntryString *block;
    size_t blockCount;
    size_t blockIndex;
} HTableString;

HTableString *htable_string_create(size_t size);
const char *htable_string_search(const HTableString *table, const char *key, size_t keyLength);
void htable_string_insert_if_not_exists(HTableString *table, const char *key, size_t keyLength, const char *value);
void htable_string_free(HTableString *table);

// #########################################################
// Hash table storing a linked list of size_t.
typedef struct ListItem {
    size_t value;
    struct ListItem *next;
} ListItem;

typedef struct EntrySizeList {
    const char *key;
    ListItem *list;
    struct EntrySizeList *next;
} EntrySizeList;

typedef struct HTableSizeList {
    EntrySizeList **buckets;
    size_t size;
    EntrySizeList *blockEntry;
    size_t blockEntryCount;
    size_t blockEntryIndex;
    ListItem *block;
    size_t blockCount;
    size_t blockIndex;
} HTableSizeList;

HTableSizeList *htable_sizelist_create(size_t size);
const ListItem *htable_sizelist_search(const HTableSizeList *table, const char *key, size_t keyLength);
void htable_sizelist_add(HTableSizeList *table, const char *key, size_t keyLength, size_t value);
void htable_sizelist_free(HTableSizeList *table);
// #########################################################


inline size_t hash(size_t tableSize, const char *key, size_t keyLength) {
    size_t hash = 0x811C9DC5; // 2166136261
    for (size_t i = 0; i < keyLength; i++) {
        hash = (hash * 31) + toupper(key[i]);
    }
    return hash & (tableSize - 1);
}

inline size_t next_power_of_two(size_t n) {
    if (n == 0)
        return 1;

    // If n is already a power of 2, return n
    if ((n & (n - 1)) == 0)
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

#endif
