#pragma once

#include <stdbool.h>

#include "data/bytecode.h"

typedef uint32_t EmitterSymbol;

typedef struct Emitter {
    Bytecode _bytecode;
    DYNAMIC_ARRAY(BytecodeLoc) _symbol_locations;
    DYNAMIC_ARRAY(BytecodeLoc) _reference_locations;
} Emitter;

Emitter emitter_new(void);

void emit_raw(Emitter* emitter, BytecodeOp op);

#define EmitArg(kind) EmitArg_##kind
#define EmitArg_int int64_t
#define EmitArg_loc EmitterSymbol
#define EmitArg_var BytecodeVar

#define emit(mnemo) emit_##mnemo
#define PROC(mnemo, code, ...)  \
    void emit_##mnemo(Emitter* emitter __VA_OPT__(, EmitArg(__VA_ARGS__) v));
FOR_OPS
#undef PROC

#define emit_sys(mnemo) emit_sys_##mnemo
#define PROC(mnemo, code, ...)  \
    void emit_sys_##mnemo(Emitter* emitter __VA_OPT__(, EmitArg(__VA_ARGS__) v));
FOR_SYSCALLS
#undef PROC

EmitterSymbol emit_symbol(Emitter* emitter);
// returns true on replace
bool emit_symbol_location(Emitter* emitter, EmitterSymbol symbol);

Bytecode emit_finish(Emitter* emitter);
