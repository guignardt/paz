#pragma once

#include <util/alloc.h>

typedef uint16_t BytecodeVar;
typedef uint64_t BytecodeLoc;

typedef union Word {
    int64_t as_int;
    BytecodeLoc as_loc;
} Word;

typedef DYNAMIC_ARRAY(uint8_t) Bytecode;

typedef enum Opcode {
    OP_NOP,
    
    OP_SYS,

    OP_CALL,
    OP_RES,
    OP_RET,

    OP_JMP,
    OP_JMPZ,
    OP_JMPNZ,

    OP_INT,
    OP_LOC,
    OP_VAR,
    OP_SET,

    OP_NEGI,

    OP_EQI,
    OP_NEI,
    OP_LEI,
    OP_LTI,
    OP_GEI,
    OP_GTI,
    
    OP_ADDI,
    OP_SUBI,
    OP_MULI,
    OP_DIVI,
    OP_REMI,
} Opcode;

typedef enum Syscall {
    SYS_NOP,
    SYS_EXIT,
    SYS_DEBUG,
} Syscall;

#define FOR_OPS                 \
    PROC(nop, OP_NOP)           \
    \
    PROC(call, OP_CALL)         \
    PROC(res, OP_RES, var)      \
    PROC(ret, OP_RET)           \
    \
    PROC(jmp, OP_JMP, loc)      \
    PROC(jmpz, OP_JMPZ, loc)    \
    PROC(jmpnz, OP_JMPNZ, loc)  \
    \
    PROC(int, OP_INT, int)      \
    PROC(loc, OP_LOC, loc)      \
    PROC(var, OP_VAR, var)      \
    PROC(set, OP_SET, var)      \
    \
    PROC(negi, OP_NEGI)         \
    \
    PROC(eqi, OP_EQI)           \
    PROC(nei, OP_NEI)           \
    PROC(lei, OP_LEI)           \
    PROC(lti, OP_LTI)           \
    PROC(gei, OP_GEI)           \
    PROC(gti, OP_GTI)           \
    \
    PROC(addi, OP_ADDI)         \
    PROC(subi, OP_SUBI)         \
    PROC(muli, OP_MULI)         \
    PROC(divi, OP_DIVI)         \
    PROC(remi, OP_REMI)         \

#define FOR_SYSCALLS            \
    PROC(nop, SYS_NOP)          \
    PROC(exit, SYS_EXIT)        \
    PROC(debug, SYS_DEBUG, var) \

typedef enum BytecodeArgKind {
    BYTECODE_ARG_NONE,
    BYTECODE_ARG_INT,
    BYTECODE_ARG_LOC,
    BYTECODE_ARG_VAR,
} BytecodeArgKind;

#define BYTECODE_ARG_KIND(kind) BYTECODE_ARG_KIND_##kind
#define BYTECODE_ARG_KIND_int BYTECODE_ARG_INT
#define BYTECODE_ARG_KIND_loc BYTECODE_ARG_LOC
#define BYTECODE_ARG_KIND_var BYTECODE_ARG_VAR

#define BytecodeArg(kind) BytecodeArg_##kind
#define BytecodeArg_int int64_t
#define BytecodeArg_loc BytecodeLoc
#define BytecodeArg_var BytecodeVar

#define bytecode_arg(kind) bytecode_arg_##kind
#define bytecode_arg_int(v) (BytecodeArg) { .kind = BYTECODE_ARG_INT, .as.int_ = v }
#define bytecode_arg_loc(v) (BytecodeArg) { .kind = BYTECODE_ARG_LOC, .as.loc = v }
#define bytecode_arg_var(v) (BytecodeArg) { .kind = BYTECODE_ARG_VAR, .as.var = v }

typedef struct BytecodeArg {
    BytecodeArgKind kind;
    union {
        int64_t int_;
        BytecodeLoc loc;
        BytecodeVar var;
    } as;
} BytecodeArg;

typedef struct BytecodeOp {
    Opcode opcode;
    Syscall syscall;
    BytecodeArg arg;
} BytecodeOp;

BytecodeOp bytecode_read(uint8_t const** cursor);
void bytecode_push(Bytecode* bc, BytecodeOp op);
