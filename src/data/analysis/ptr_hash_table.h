#pragma once

#include <stdbool.h>

#include "util/alloc.h"

// only supports nonzero/non-null keys
typedef struct PtrHashTable {
    BOXED(void) _data;
    size_t _len;
    size_t _entry_count;
    size_t _value_size;
} PtrHashTable;

PtrHashTable ptr_hash_table_new(size_t value_size);
void ptr_hash_table_free(OWNED(PtrHashTable) table);

const void* ptr_hash_table_get(PtrHashTable table, intptr_t key);
void* ptr_hash_table_get_mut(PtrHashTable* table, intptr_t key);
// returns true on replace
bool ptr_hash_table_insert(PtrHashTable* table, intptr_t key, void const* value);

typedef struct PtrHashTableIt {
    intptr_t const* _keys;
    void const* _values;
    size_t _i;
    size_t _entry_count;
    size_t _value_size;
} PtrHashTableIt;

PtrHashTableIt ptr_hash_table_it(PtrHashTable table);
void const* ptr_hash_table_it_next(PtrHashTableIt* it, OUT(intptr_t) key);
