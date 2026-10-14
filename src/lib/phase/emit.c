#include "phase/emit.h"
#include "diagnostic/report.h"

Emitter emitter_new(void) {
    return (Emitter){
        ._bytecode = {0},
        ._symbol_locations = {0},
    };
}

void emit_raw(Emitter* e, BytecodeOp op) {
    bytecode_push(&e->_bytecode, op);
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

void emit_symbol_location(Emitter* e, EmitterSymbol sym) {
    e->_symbol_locations.data[sym] = e->_bytecode.len;
}

Bytecode emit_finish(Emitter* e) {
    for (EmitterSymbol sym; sym < e->_symbol_locations.len; sym++) {
        BytecodeLoc loc = e->_symbol_locations.data[sym];
        if (loc == -1) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("emitter error: unresolved symbol %u", (unsigned int)sym);
            report_end();
        }

        uint64_t loc_bits = loc;
        uint8_t* cursor = e->_bytecode.data + loc;
        for (size_t i = 0; i < 7; i++) {
            *cursor = loc_bits;
            cursor++;
            loc_bits >>= 8;
        }
    }

    free(e->_symbol_locations.data);
    return e->_bytecode;
}
