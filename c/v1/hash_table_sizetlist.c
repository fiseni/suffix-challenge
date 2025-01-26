#include "common.h"
#include "hash_table.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose hash table.
* It is tailored for our scenario and it is safe to use only within this context.
*/

static bool str_equals_same_length(const char *s1, const char *s2, size_t length) {
    assert(s1);
    assert(s2);

    for (size_t i = 0; i < length; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

HTableSizeTList *htable_sizetlist_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTableSizeTList *table = malloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = malloc(sizeof(*table->buckets) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }
    table->blockEntryIndex = 0;
    table->blockEntryCount = size;
    table->blockEntry = malloc(sizeof(*table->blockEntry) * table->blockEntryCount);
    CHECK_ALLOC(table->blockEntry);
    table->blockIndex = 0;
    table->blockCount = size;
    table->block = malloc(sizeof(*table->block) * table->blockCount);
    CHECK_ALLOC(table->block);
    return table;
}

const ListItem *htable_sizetlist_search(const HTableSizeTList *table, const char *key, size_t keyLength) {
    size_t index = hash(table->size, key, keyLength);
    EntrySizeTList *entry = table->buckets[index];
    while (entry) {
        if (str_equals_same_length(entry->key, key, keyLength)) {
            return entry->list;
        }
        entry = entry->next;
    }
    return NULL;
}

static void linked_list_add(HTableSizeTList *table, EntrySizeTList *entry, size_t value) {
    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockIndex < table->blockCount);

    ListItem *newItem = &table->block[table->blockIndex++];
    newItem->value = value;
    newItem->next = entry->list;
    entry->list = newItem;
}

void htable_sizetlist_add(HTableSizeTList *table, const char *key, size_t keyLength, size_t value) {
    size_t index = hash(table->size, key, keyLength);
    EntrySizeTList *entry = table->buckets[index];

    while (entry) {
        // We know this table is always used with same key length.
        if (str_equals_same_length(entry->key, key, keyLength)) {
            linked_list_add(table, entry, value);
            return;
        }
        entry = entry->next;
    }

    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockEntryIndex < table->blockEntryCount);

    EntrySizeTList *newEntry = &table->blockEntry[table->blockEntryIndex++];
    newEntry->key = key;
    newEntry->list = NULL;
    linked_list_add(table, newEntry, value);
    newEntry->next = table->buckets[index];
    table->buckets[index] = newEntry;
}

void htable_sizetlist_free(HTableSizeTList *table) {
    free(table->blockEntry);
    free(table->block);
    free(table->buckets);
    free(table);
}
