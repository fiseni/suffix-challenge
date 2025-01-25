#ifndef HASH_TABLE_H
#define HASH_TABLE_H

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

#endif
