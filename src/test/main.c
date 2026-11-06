#include <stdio.h>
#include <string.h>

#include "diagnostic/report.h"

#include "data/analysis/id_hash_table.h"

#include "phase/assemble.h"
#include "phase/disassemble.h"

#include "testing.h"

int test_hash_table(void) {
    IdHashTable table = id_hash_table_new(sizeof(int));
    int val = 2;    TEST_ASSERT(!id_hash_table_insert(&table, 1, &val));
    val = 10;       TEST_ASSERT(!id_hash_table_insert(&table, 5, &val));
    val = 18;       TEST_ASSERT(!id_hash_table_insert(&table, 9, &val));
    val = 26;       TEST_ASSERT(!id_hash_table_insert(&table, 13, &val));
    val = 34;       TEST_ASSERT(!id_hash_table_insert(&table, 17, &val));

    TEST_ASSERT(!id_hash_table_get(table, 11));
    TEST_ASSERT(id_hash_table_get(table, 1));
    TEST_ASSERT(id_hash_table_get(table, 5));
    TEST_ASSERT(id_hash_table_get(table, 9));
    TEST_ASSERT(id_hash_table_get(table, 13));
    TEST_ASSERT(id_hash_table_get(table, 17));

    bool seen_key[5] = {0};
    Id key;
    int const* p_value;
    for (
        IdHashTableIt it = id_hash_table_it(table);
        (p_value = id_hash_table_it_next(&it, &key));
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

int test_assemble(void) {
    char const* assembly[] = {
       "    nop",
       "    res %1",
       "    int #4",
       "    set %0",
       ":start",
       "    int #3",
       "    int #-2",
       "    nei",
       "    jmpnz :start",
    };
    uint8_t expected_bytecode[] = {
            OP_NOP,
            OP_RES, 1, 0,
            OP_INT, 4, 0, 0, 0, 0, 0, 0, 0,
            OP_SET, 0, 0,
        [16] =
            OP_INT, 3, 0, 0, 0, 0, 0, 0, 0,
            OP_INT, -2, -1, -1, -1, -1, -1, -1, -1,
            OP_NEI,
            OP_JMPNZ, 16, 0, 0, 0, 0, 0, 0, 0,
    };
    Bytecode computed_bytecode = assemble(assembly, sizeof(assembly) / sizeof(char const*));

    TEST_ASSERT(report_count(SEVERITY_ERROR) == 0);
    TEST_ASSERT(computed_bytecode.len == sizeof(expected_bytecode));
    TEST_ASSERT(!memcmp(expected_bytecode, computed_bytecode.data, sizeof(expected_bytecode)));

    free(computed_bytecode.data);

    return 0;
}

int test_disassemble(void) {
    Bytecode bytecode = {0};
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_RES, .arg = { .kind = BYTECODE_ARG_VAR, .as.var_ = 2 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_INT, .arg = { .kind = BYTECODE_ARG_INT, .as.int_ = 5 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_SET, .arg = { .kind = BYTECODE_ARG_VAR, .as.var_ = 1 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_VAR, .arg = { .kind = BYTECODE_ARG_VAR, .as.var_ = 1 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_INT, .arg = { .kind = BYTECODE_ARG_INT, .as.int_ = 0 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_GTI });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_JMPNZ, .arg = { .kind = BYTECODE_ARG_LOC, .as.loc_ = 62 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_VAR, .arg = { .kind = BYTECODE_ARG_VAR, .as.var_ = 1 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_INT, .arg = { .kind = BYTECODE_ARG_INT, .as.int_ = -1 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_ADDI });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_SET, .arg = { .kind = BYTECODE_ARG_VAR, .as.var_ = 1 } });
    bytecode_push_op(&bytecode, (BytecodeOp) { .opcode = OP_JMP, .arg = { .kind = BYTECODE_ARG_LOC, .as.loc_ = 15 } });
    
    char expected_disassembly[] =
        "    res %2\n"
        "    int #5\n"
        "    set %1\n"
        ":L15\n"
        "    var %1\n"
        "    int #0\n"
        "    gti\n"
        "    jmpnz :L62\n"
        "    var %1\n"
        "    int #-1\n"
        "    addi\n"
        "    set %1\n"
        "    jmp :L15\n"
        ":L62\n"
    ;

    BOXED(char) computed_disassembly = disassemble(bytecode);

    TEST_ASSERT(!strcmp(computed_disassembly, expected_disassembly));

    free(computed_disassembly);

    return 0;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
    set_reporter(default_reporter());

    RUN_TEST(test_hash_table);
    RUN_TEST(test_assemble);
    RUN_TEST(test_disassemble);
    finish_tests();

    return 0;
}
