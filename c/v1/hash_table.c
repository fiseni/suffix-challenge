#include "allocator.h"
#include "common.h"
#include "hash_table.h"

/* Fati Iseni
* DO NOT use this implementation as a general purpose hash table.
* It is tailored for our scenario and it is safe to use only within this context.
*/

static inline size_t hash(size_t tableSize, const StringView *key) {
    size_t hash = 0x811C9DC5; // 2166136261
    for (size_t i = 0; i < key->length; i++) {
        hash = (hash * 31) + key->data[i];
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

static bool str_equals(const StringView *s1, const StringView *s2) {
    if (s1->length != s2->length) {
        return false;
    }
    for (size_t i = 0; i < s1->length && i < s2->length; i++) {
        if (s1->data[i] != s2->data[i]) {
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
    HTable *table = allocator_alloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = allocator_alloc(sizeof(*table->buckets) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }

    table->blockEntriesIndex = 0;
    table->blockEntriesCount = size;
    table->blockEntries = allocator_alloc(sizeof(*table->blockEntries) * table->blockEntriesCount);
    CHECK_ALLOC(table->blockEntries);

    return table;
}

// Used to avoid re-computing the hash value.
static const StringView *search_internal(const HTable *table, const StringView *key, size_t index) {
    Entry *entry = table->buckets[index];
    while (entry) {
        if (str_equals(&entry->key, key)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const StringView *htable_search(const HTable *table, const StringView *key) {
    if (table == NULL) {
        return false;
    }

    size_t index = hash(table->size, key);
    return search_internal(table, key, index);
}

void htable_insert_if_not_exists(HTable *table, const StringView *key, const StringView *value) {
    size_t index = hash(table->size, key);
    if (search_internal(table, key, index)) {
        return;
    }

    // In our scenario, we'll never add more items than the initially size passed during table creation.
    // So, we're sure that we won't run out of space. No need to malloc again.
    assert(table->blockEntriesIndex < table->blockEntriesCount);

    Entry *new_entry = &table->blockEntries[table->blockEntriesIndex++];
    new_entry->key = *key;
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
