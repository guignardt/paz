#pragma once

#include "data/ast/binding.h"
#include "data/ast/expression.h"
#include "data/ast/identifier.h"
#include "data/ast/node.h"
#include "data/ast/pattern.h"
#include "data/ast/storage.h"
#include "data/ast/type.h"

typedef struct AstModule {
    AstBinding* bindings;
} AstModule;

typedef struct AstProgram {
    AstModule* root;
} AstProgram;

void debug_ast_module(AstModule const* v);
void debug_ast_program(AstProgram const* v);
