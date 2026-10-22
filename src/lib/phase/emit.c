#include "phase/emit.h"
#include "diagnostic/report.h"

Emitter emitter_new(void) {
    return (Emitter){
        ._bytecode = {0},
        ._symbol_locations = {0},
        ._reference_locations = {0},
    };
}

void emit_raw(Emitter* e, BytecodeOp op) {
    bytecode_push_op(&e->_bytecode, op);
    if (op.arg.kind == BYTECODE_ARG_LOC) {
        BytecodeLoc loc = e->_bytecode.len - sizeof(BytecodeLoc);
        DYNAMIC_ARRAY_PUSH(&e->_reference_locations, loc);
    }
}

#define emit_arg(kind) bytecode_arg(kind)

#define PROC(mnemo, code, ...)  \
    void emit_##mnemo(Emitter* e __VA_OPT__(, EmitArg(__VA_ARGS__) v)) {    \
        BytecodeOp op = {                                                       \
            .opcode = code,                                                     \
            __VA_OPT__(.arg = emit_arg(__VA_ARGS__)(v))                     \
        };                                                                      \
        emit_raw(e, op);                                       \
    }
FOR_OPS
#undef PROC

#define PROC(mnemo, code, ...)  \
    void emit_sys_##mnemo(Emitter* e __VA_OPT__(, EmitArg(__VA_ARGS__) v)) {    \
        BytecodeOp op = {                                                           \
            .opcode = OP_SYS,                                                       \
            .syscall = code,                                                        \
            __VA_OPT__(.arg = emit_arg(__VA_ARGS__)(v))                         \
        };                                                                          \
        emit_raw(e, op);                                           \
    }
FOR_SYSCALLS
#undef PROC

EmitterSymbol emit_symbol(Emitter* e) {
    EmitterSymbol sym = e->_symbol_locations.len;
    DYNAMIC_ARRAY_PUSH(&e->_symbol_locations, -1);
    return sym;
}

bool emit_symbol_location(Emitter* e, EmitterSymbol sym) {
    bool replace = e->_symbol_locations.data[sym] != -1;
    e->_symbol_locations.data[sym] = e->_bytecode.len;
    return replace;
}

Bytecode emit_finish(Emitter* e) {
    for (EmitterSymbol sym = 0; sym < e->_symbol_locations.len; sym++) {
        BytecodeLoc loc = e->_symbol_locations.data[sym];
        if (loc == -1) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("emitter error: unresolved symbol %u", (unsigned int)sym);
            report_end();
        }
    }

    for (size_t i = 0; i < e->_reference_locations.len; i++) {
        BytecodeLoc loc = e->_reference_locations.data[i];
        uint8_t* wp = e->_bytecode.data + loc;
        uint8_t const* rp = wp;
        EmitterSymbol sym = bytecode_read_uint64(&rp);
        uint64_t sym_loc_bits = e->_symbol_locations.data[sym];
        for (size_t i = 0; i < 8; i++) {
            *wp = sym_loc_bits;
            wp++;
            sym_loc_bits >>= 8;
        }
    }

    free(e->_symbol_locations.data);
    free(e->_reference_locations.data);
    return e->_bytecode;
}
