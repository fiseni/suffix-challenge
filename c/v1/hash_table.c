#include "common.h"
#include "hash_table.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose hash table.
* It is tailored for our scenario and it is safe to use only within this context.
*/

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

HTable *htable_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTable *table = malloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = malloc(sizeof(*table->buckets) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }

    table->blockEntriesIndex = 0;
    table->blockEntriesCount = size;
    table->blockEntries = malloc(sizeof(*table->blockEntries) * table->blockEntriesCount);
    CHECK_ALLOC(table->blockEntries);

    return table;
}

// Used to avoid re-computing the hash value.
static bool search_internal(const HTable *table, const char *key, size_t keyLength, size_t index, size_t *outValue) {
    Entry *entry = table->buckets[index];
    while (entry) {
        if (str_equals_one_length(entry->key, key, keyLength)) {
            *outValue = entry->value;
            return true;
        }
        entry = entry->next;
    }
    return false;
}

bool htable_search(const HTable *table, const char *key, size_t keyLength, size_t *outValue) {
    if (table == NULL) {
        return false;
    }

    size_t index = hash(table->size, key, keyLength);
    return search_internal(table, key, keyLength, index, outValue);
}

void htable_insert_if_not_exists(HTable *table, const char *key, size_t keyLength, size_t value) {
    size_t index = hash(table->size, key, keyLength);
    size_t existing_value;
    if (search_internal(table, key, keyLength, index, &existing_value)) {
        return;
    }

    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockEntriesIndex < table->blockEntriesCount);

    Entry *new_entry = &table->blockEntries[table->blockEntriesIndex++];
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

void htable_free(HTable *table) {
    if (table) {
        free(table->blockEntries);
        free(table->buckets);
        free(table);
    }
}
