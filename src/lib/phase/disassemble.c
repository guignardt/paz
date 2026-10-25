#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "phase/disassemble.h"

typedef DYNAMIC_ARRAY(char) StringBuf;

static void sort_locs(BytecodeLoc* data, size_t len);
static size_t dedup_locs(BytecodeLoc* data, size_t len);
static void extend_str(StringBuf* b, char const* s);

BOXED(char) disassemble(Bytecode bytecode) {
    uint8_t const* end = bytecode.data + bytecode.len;

    DYNAMIC_ARRAY(BytecodeLoc) locs = {0};
    uint8_t const* rp = bytecode.data;
    while (rp < end) {
        BytecodeOp op = bytecode_read_op(&rp);
        if (op.arg.kind == BYTECODE_ARG_LOC) {
            DYNAMIC_ARRAY_PUSH(&locs, op.arg.as.loc);
        }
    }

    sort_locs(locs.data, locs.len);
    locs.len = dedup_locs(locs.data, locs.len);

    StringBuf text = {0};
    size_t loc_idx = 0;
    rp = bytecode.data;
    while (rp <= end) {
        char buf[64];

        if (loc_idx < locs.len && rp - bytecode.data >= locs.data[loc_idx]) {
            snprintf(buf, 64, ":L%llu\n", (unsigned long long)locs.data[loc_idx]);
            extend_str(&text, buf);
            loc_idx++;
        }

        if (rp == end) break;

        BytecodeOp op = bytecode_read_op(&rp);
        extend_str(&text, "    ");
        extend_str(&text, opcode_mnemos[op.opcode].string);

        if (op.opcode == OP_SYS) {
            extend_str(&text, syscall_mnemos[op.opcode].string);
        }

        switch (op.arg.kind) {
            case BYTECODE_ARG_NONE: break;
            case BYTECODE_ARG_INT:
                snprintf(buf, 64, " #%lld", (long long)(op.arg.as.int_));
                extend_str(&text, buf);
                break;
            case BYTECODE_ARG_LOC:
                snprintf(buf, 64, " :L%llu", (unsigned long long)(op.arg.as.loc));
                extend_str(&text, buf);
                break;
            case BYTECODE_ARG_VAR:
                snprintf(buf, 64, " %%%u", (unsigned int)(op.arg.as.var));
                extend_str(&text, buf);
                break;
        }

        DYNAMIC_ARRAY_PUSH(&text, '\n');
    }

    DYNAMIC_ARRAY_PUSH(&text, '\0');

    return text.data;
}

static void sort_locs(BytecodeLoc* data, size_t len) {
    if (len < 2) return;

    if (len == 2) {
        if (data[0] > data[1]) {
            BytecodeLoc tmp = data[0];
            data[0] = data[1];
            data[1] = tmp;
        }
        return;
    }

    size_t pivot_idx = len / 2 + 1;
    size_t second_start = pivot_idx + 1;
    BytecodeLoc pivot = data[pivot_idx];
    for (size_t i = 0; i < pivot_idx; i++) {
        BytecodeLoc x = data[i];
        if (x <= pivot) continue;

        // 0 <= i < pivot_idx => pivot_idx >= 1
        data[i] = data[pivot_idx - 1];
        data[pivot_idx - 1] = pivot;
        data[pivot_idx] = x;
        pivot_idx--;
    }

    for (size_t i = second_start; i < len; i++) {
        BytecodeLoc x = data[i];
        if (x >= pivot) continue;

        // pivot_idx < i < len => pivot_idx + 1 < len
        data[i] = data[pivot_idx + 1];
        data[pivot_idx + 1] = pivot;
        data[pivot] = x;
        pivot_idx++;
    }

    size_t low_end = pivot_idx;
    size_t high_start = pivot_idx + 1;

    sort_locs(data, low_end);
    sort_locs(data + high_start, len - high_start);
}

static size_t dedup_locs(BytecodeLoc* data, size_t len) {
    if (len == 0) return 0;

    size_t w = 0;
    for (size_t i = 1; i < len; i++) {
        if (data[i] != data[w]) {
            w++;
            data[w] = data[i];
        }
    }
    return w + 1;
}

static void extend_str(StringBuf* b, char const* s) {
    size_t slen = strlen(s);
    reserve(ERASE2(&b->data), &b->cap, b->len + slen, 1);
    memmove(b->data + b->len, s, slen);
    b->len += slen;
}
