#include "data/bytecode.h"

static void push_uint8(Bytecode* bc, uint8_t v);
static void push_uint16(Bytecode* bc, uint16_t v);
static void push_uint64(Bytecode* bc, uint64_t v);

static uint8_t read_uint8(uint8_t const** bc);
static uint16_t read_uint16(uint8_t const** bc);
static uint64_t read_uint64(uint8_t const** bc);

void bytecode_push(Bytecode* bc, BytecodeOp op) {
    push_uint8(bc, op.opcode);
    if (op.opcode == OP_SYS) {
        push_uint8(bc, op.syscall);
    }
    switch (op.arg.kind) {
        case BYTECODE_ARG_NONE: break;
        case BYTECODE_ARG_INT: push_uint64(bc, op.arg.as.int_); break;
        case BYTECODE_ARG_LOC: push_uint64(bc, op.arg.as.loc); break;
        case BYTECODE_ARG_VAR: push_uint16(bc, op.arg.as.var); break;
    }
}

#define read_int read_uint64
#define read_loc read_uint64
#define read_var read_uint16

BytecodeOp bytecode_read(uint8_t const** bc) {
    // FIXME: what to do

    BytecodeOp op = {0};
    op.opcode = read_uint8(bc);

    #define ARG_KIND_int BYTECODE_ARG_INT
    #define ARG_KIND_loc BYTECODE_ARG_LOC
    #define ARG_KIND_var BYTECODE_ARG_VAR

    #define ARG_FIELD_int op.arg.as.int_
    #define ARG_FIELD_loc op.arg.as.loc
    #define ARG_FIELD_var op.arg.as.var

    #define PROC(mnemo, code, ...)                                  \
        __VA_OPT__(                                                 \
            case code:                                              \
                op.arg.kind = ARG_KIND_##__VA_ARGS__;               \
                ARG_FIELD_##__VA_ARGS__ = read_##__VA_ARGS__(bc);   \
                break;                                              \
        )
    
    switch (op.opcode) {
        FOR_OPS

        case OP_SYS:
            op.syscall = read_uint8(bc);
            switch (op.syscall) {
                FOR_SYSCALLS

                default:
                    op.syscall = SYS_NOP;
                    break;
            }
            break;

        default:
            op.opcode = OP_NOP;
            break;
    }

    return op;
}

static void push_uint8(Bytecode* bc, uint8_t v) {
    DYNAMIC_ARRAY_PUSH(bc, v);
}

static void push_uint16(Bytecode* bc, uint16_t v) {
    push_uint8(bc, v);
    push_uint8(bc, v >> 8);
}

static void push_uint64(Bytecode* bc, uint64_t v) {
    push_uint8(bc, v);
    push_uint8(bc, v >> 8);
    push_uint8(bc, v >> 16);
    push_uint8(bc, v >> 24);
    push_uint8(bc, v >> 32);
    push_uint8(bc, v >> 40);
    push_uint8(bc, v >> 48);
    push_uint8(bc, v >> 56);
}

BytecodeOp bytecode_read(uint8_t const** cursor);

static uint8_t read_uint8(uint8_t const** bc) {
    return *((*bc)++);
}

static uint16_t read_uint16(uint8_t const** bc) {
    uint16_t v0 = read_uint8(bc);
    uint16_t v1 = read_uint8(bc);
    return v0 | (v1 << 8);
}

static uint64_t read_uint64(uint8_t const** bc) {
    uint64_t v0 = read_uint8(bc);
    uint64_t v1 = read_uint8(bc);
    uint64_t v2 = read_uint8(bc);
    uint64_t v3 = read_uint8(bc);
    uint64_t v4 = read_uint8(bc);
    uint64_t v5 = read_uint8(bc);
    uint64_t v6 = read_uint8(bc);
    uint64_t v7 = read_uint8(bc);
    return v0 | (v1 << 8) | (v2 << 16) | (v3 << 24)
        | (v4 << 32) | (v5 << 40) | (v6 << 48) | (v7 << 56);
}
