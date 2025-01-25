#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose hash table.
* It is tailored for our scenario and it is safe to use only within this context.
*/

static size_t hash(const HTableString *table, const char *key, size_t keyLength) {
    size_t hash = 0x811C9DC5; // 2166136261
    for (size_t i = 0; i < keyLength; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash & (table->size - 1);
}

static bool str_equals_one_length(const char *str1, const char *str2, size_t str2Length) {
    return strcasecmp(str1, str2) == 0;

    for (size_t i = 0; i < str2Length; i++) {
        if (str1[i] == '\0' || str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

HTableString *htable_string_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTableString *table = malloc(sizeof(*table));
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
static const char *search_internal(const HTableString *table, const char *key, size_t keyLength, size_t index) {
    EntryString *entry = table->buckets[index];
    while (entry) {
        //if (strcmp(entry->key, key) == 0) {
        //if (strncmp(entry->key, key, keyLength) == 0) {
        if (str_equals_one_length(entry->key, key, keyLength)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const char *htable_string_search(const HTableString *table, const char *key, size_t keyLength) {
    size_t index = hash(table, key, keyLength);
    return search_internal(table, key, keyLength, index);
}

void htable_string_insert_if_not_exists(HTableString *table, const char *key, size_t keyLength, const char *value) {
    size_t index = hash(table, key, keyLength);
    const char *existing_value = search_internal(table, key, keyLength, index);
    if (existing_value) {
        return;
    }

    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockIndex < table->blockCount);

    EntryString *new_entry = &table->block[table->blockIndex++];
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

void htable_string_free(HTableString *table) {
    free(table->block);
    free(table->buckets);
    free(table);
}
