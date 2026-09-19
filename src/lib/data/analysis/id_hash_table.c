#include <stdlib.h>
#include <string.h>

#include "data/analysis/id_hash_table.h"

uint32_t hash_id(uint32_t x) {
    // FIXME: better hash function?

    // https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

IdHashTable id_hash_table_new(size_t value_size) {
    return (IdHashTable) {
        ._data = NULL,
        ._entry_count = 0,
        ._len = 0,
        ._value_size = value_size,
    };
}

void id_hash_table_free(OWNED(IdHashTable) table) {
    free(table->_data);
}

void const* id_hash_table_get(IdHashTable table, Id key) {
    return id_hash_table_get_mut(&table, key);
}

void* id_hash_table_get_mut(IdHashTable* p_table, Id key) {
    IdHashTable table = *p_table;

    if (table._entry_count == 0) return NULL;

    Id* keys = table._data;
    size_t start_i = hash_id(key) % table._entry_count;
    size_t i = start_i;
    while (1) {
        Id entry_key = keys[i];
        if (entry_key == ID_SENTINEL) {
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
bool ptr_hash_table_insert_vacant(IdHashTable* p_table, Id key, void const* value) {
    IdHashTable table = *p_table;

    Id* keys = table._data;

    size_t key_hash = hash_id(key);
    size_t start_i = key_hash % table._entry_count;
    size_t i = start_i;
    bool replaced;
    while (1) {
        Id* entry_key = &keys[i];
        if (*entry_key == ID_SENTINEL) {
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

bool id_hash_table_insert(IdHashTable* p_table, Id key, void const* value) {
    IdHashTable table = *p_table;
    
    // we first resize if needed, ensuring there is room
    if (table._len >= MAX_LOAD_FACTOR * table._entry_count) {
        size_t new_entry_count = (table._entry_count <= 2) ? 4 : (2 * table._entry_count);
        void* new_data = alloc_or_exit(new_entry_count * (sizeof(Id) + table._value_size));
        memset(new_data, 0, new_entry_count * sizeof(Id));
        IdHashTable new_table = {
            ._data = new_data,
            ._entry_count = new_entry_count,
            ._len = 0,
            ._value_size = table._value_size,
        };

        Id const* keys = table._data;

        void const* values = keys + table._entry_count;
        for (size_t i = 0; i < table._entry_count; i++) {
            Id key = keys[i];
            if (key == ID_SENTINEL) continue;
            void const* value = values + i * table._value_size;
            ptr_hash_table_insert_vacant(&new_table, key, value);
        }

        id_hash_table_free(&table);
        table = new_table;
    }

    bool replaced = ptr_hash_table_insert_vacant(&table, key, value);

    *p_table = table;

    return replaced;
}

IdHashTableIt id_hash_table_it(IdHashTable table) {
    Id* keys = table._data;
    return (IdHashTableIt) {
        ._keys = keys,
        ._values = keys + table._entry_count,
        ._i = 0,
        ._entry_count = table._entry_count,
        ._value_size = table._value_size,
    };
}

void* id_hash_table_it_next(IdHashTableIt* it, OUT(Id) key) {
    for (; it->_i < it->_entry_count; it->_i++) {
        if (it->_keys[it->_i]) {
            size_t i = it->_i++;
            if (key) *key = it->_keys[i];
            return it->_values + i * it->_value_size;
        }
    }
    return NULL;
}
