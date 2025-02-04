#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>

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

#endif
