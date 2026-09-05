#include <stdio.h>

#include "diagnostic/log.h"
#include "data/analysis/ptr_hash_table.h"

#include "testing.h"

int test_ptr_hash_table(void) {
    PtrHashTable table = ptr_hash_table_new(sizeof(int));
    int val = 2;    TEST_ASSERT(!ptr_hash_table_insert(&table, 1, &val));
    val = 10;       TEST_ASSERT(!ptr_hash_table_insert(&table, 5, &val));
    val = 18;       TEST_ASSERT(!ptr_hash_table_insert(&table, 9, &val));
    val = 26;       TEST_ASSERT(!ptr_hash_table_insert(&table, 13, &val));
    val = 34;       TEST_ASSERT(!ptr_hash_table_insert(&table, 17, &val));

    TEST_ASSERT(!ptr_hash_table_get(table, 11));
    TEST_ASSERT(ptr_hash_table_get(table, 1));
    TEST_ASSERT(ptr_hash_table_get(table, 5));
    TEST_ASSERT(ptr_hash_table_get(table, 9));
    TEST_ASSERT(ptr_hash_table_get(table, 13));
    TEST_ASSERT(ptr_hash_table_get(table, 17));

    bool seen_key[5] = {0};
    intptr_t key;
    int const* p_value;
    for (
        PtrHashTableIt it = ptr_hash_table_it(table);
        (p_value = ptr_hash_table_it_next(&it, &key));
    ) {
        TEST_ASSERT(1 <= key && key <= 17 && key % 4 == 1);
        size_t i = (key - 1) / 4;
        TEST_ASSERT(!seen_key[i]);
        seen_key[i] = true;
        TEST_ASSERT(p_value);
        TEST_ASSERT(*p_value == 2 * key);
    }

    return 0;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
    RUN_TEST(test_ptr_hash_table);
    finish_tests();
    return 0;
}
