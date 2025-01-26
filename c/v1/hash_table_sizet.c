#include "common.h"
#include "hash_table.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose hash table.
* It is tailored for our scenario and it is safe to use only within this context.
*/

static bool str_equals_one_length(const char *s1, const char *s2, size_t s2Length) {
    assert(s1);
    assert(s2);

    for (size_t i = 0; i < s2Length; i++) {
        if (s1[i] == '\0' || s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

HTableSizeT *htable_sizet_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTableSizeT *table = malloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = malloc(sizeof(*table->buckets) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }
    table->blockIndex = 0;
    table->blockCount = size;
    table->block = malloc(sizeof(*table->block) * table->blockCount);
    CHECK_ALLOC(table->block);
    return table;
}

// Used to avoid re-computing the hash value.
static size_t search_internal(const HTableSizeT *table, const char *key, size_t keyLength, size_t index) {
    EntrySizeT *entry = table->buckets[index];
    while (entry) {
        if (str_equals_one_length(entry->key, key, keyLength)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return MAX_SIZE_T_VALUE;
}

size_t htable_sizet_search(const HTableSizeT *table, const char *key, size_t keyLength) {
    size_t index = hash(table->size, key, keyLength);
    return search_internal(table, key, keyLength, index);
}

void htable_sizet_insert_if_not_exists(HTableSizeT *table, const char *key, size_t keyLength, size_t value) {
    size_t index = hash(table->size, key, keyLength);
    size_t existing_value = search_internal(table, key, keyLength, index);
    if (existing_value != MAX_SIZE_T_VALUE) {
        return;
    }

    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockIndex < table->blockCount);

    EntrySizeT *new_entry = &table->block[table->blockIndex++];
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

void htable_sizet_free(HTableSizeT *table) {
    free(table->block);
    free(table->buckets);
    free(table);
}
