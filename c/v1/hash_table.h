#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include "common.h"

typedef struct Entry {
    StringView key;
    const StringView *value;
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
const StringView *htable_search(const HTable *table, const StringView *key);
void htable_insert_if_not_exists(HTable *table, const StringView *key, const StringView *value);
void htable_free(HTable *table);

#endif
