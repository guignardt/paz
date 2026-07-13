#include <stdlib.h>
#include <string.h>

#include "data/analysis.h"

size_t hash_ptr(size_t key) {
    // based on https://gist.github.com/badboy/6267743.

    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

PtrHashTable ptr_hash_table_new(size_t value_size) {
    return (PtrHashTable) {
        ._data = NULL,
        ._entry_count = 0,
        ._len = 0,
        ._value_size = value_size,
    };
}

void ptr_hash_table_free(OWNED(PtrHashTable) table) {
    free(table->_data);
}

void const* ptr_hash_table_get(PtrHashTable table, intptr_t key) {
    return ptr_hash_table_get_mut(&table, key);
}

void* ptr_hash_table_get_mut(PtrHashTable* p_table, intptr_t key) {
    PtrHashTable table = *p_table;

    if (table._entry_count == 0) return NULL;

    intptr_t* keys = table._data;
    size_t start_i = hash_ptr(key) % table._entry_count;
    size_t i = start_i;
    while (1) {
        intptr_t entry_key = keys[i];
        if (!entry_key) {
            return NULL;
        }
        if (entry_key == key) {
            break;
        }
        i++;
        if (i >= table._entry_count) i = 0;
        if (i == start_i) return NULL;
    }

    void* values = keys + table._entry_count;
    return values + i * table._value_size;
}

// assumes the hash map contains a vacant entry
bool ptr_hash_table_insert_vacant(PtrHashTable* p_table, intptr_t key, void const* value) {
    PtrHashTable table = *p_table;

    intptr_t* keys = table._data;

    size_t key_hash = hash_ptr(key);
    size_t start_i = key_hash % table._entry_count;
    size_t i = start_i;
    bool replaced;
    while (1) {
        intptr_t* entry_key = &keys[i];
        if (!*entry_key) {
            *entry_key = key;
            table._len++;
            replaced = false;
            break;
        }
        if (*entry_key == key) {
            replaced = true;
            break;
        }
        i++;
        if (i >= table._entry_count) i = 0;
        // we will break as we must find a vacant entry at some point
    }

    void* values = keys + table._entry_count;
    if (table._value_size) {
        memmove(values + i * table._value_size, value, table._value_size);
    }

    *p_table = table;

    return replaced;
}

#define MAX_LOAD_FACTOR 0.75

bool ptr_hash_table_insert(PtrHashTable* p_table, intptr_t key, void const* value) {
    PtrHashTable table = *p_table;
    
    // we first resize if needed, ensuring there is room
    if (table._len >= MAX_LOAD_FACTOR * table._entry_count) {
        size_t new_entry_count = (table._entry_count <= 2) ? 4 : (2 * table._entry_count);
        void* new_data = alloc_or_exit(new_entry_count * (sizeof(intptr_t) + table._value_size));
        memset(new_data, 0, new_entry_count * sizeof(intptr_t));
        PtrHashTable new_table = {
            ._data = new_data,
            ._entry_count = new_entry_count,
            ._len = 0,
            ._value_size = table._value_size,
        };

        intptr_t const* keys = table._data;

        void const* values = keys + table._entry_count;
        for (size_t i = 0; i < table._entry_count; i++) {
            intptr_t key = keys[i];
            if (!key) continue;
            void const* value = values + i * table._value_size;
            ptr_hash_table_insert_vacant(&new_table, key, value);
        }

        ptr_hash_table_free(&table);
        table = new_table;
    }

    bool replaced = ptr_hash_table_insert_vacant(&table, key, value);

    *p_table = table;

    return replaced;
}

PtrHashTableIt ptr_hash_table_it(PtrHashTable table) {
    intptr_t const* keys = table._data;
    return (PtrHashTableIt) {
        ._keys = keys,
        ._values = keys + table._entry_count,
        ._i = 0,
        ._entry_count = table._entry_count,
        ._value_size = table._value_size,
    };
}

void const* ptr_hash_table_it_next(PtrHashTableIt* it, OUT(intptr_t) key) {
    for (; it->_i < it->_entry_count; it->_i++) {
        if (it->_keys[it->_i]) {
            size_t i = it->_i++;
            if (key) *key = it->_keys[i];
            return it->_values + i * it->_value_size;
        }
    }
    return NULL;
}
