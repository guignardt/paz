#pragma once

#include <stdbool.h>

#include "util/id.h"
#include "util/alloc.h"

// only supports nonzero/non-null keys
typedef struct IdHashTable {
    BOXED(void) _data;
    size_t _len;
    size_t _entry_count;
    size_t _value_size;
} IdHashTable;

IdHashTable id_hash_table_new(size_t value_size);
void id_hash_table_free(OWNED(IdHashTable) table);

const void* id_hash_table_get(IdHashTable table, Id key);
void* id_hash_table_get_mut(IdHashTable* table, Id key);
// returns true on replace
bool id_hash_table_insert(IdHashTable* table, Id key, void const* value);

typedef struct IdHashTableIt {
    Id* _keys;
    void* _values;
    size_t _i;
    size_t _entry_count;
    size_t _value_size;
} IdHashTableIt;

IdHashTableIt id_hash_table_it(IdHashTable table);
void* id_hash_table_it_next(IdHashTableIt* it, OUT(Id) key);
