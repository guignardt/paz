#pragma once

#include "data/ast/binding_set.h"
#include "data/ast/expression.h"
#include "data/ast/identifier.h"
#include "data/ast/pattern.h"

typedef struct AstModule {
    AstBindingSetId binding_sets; // list
} AstModule;

typedef struct AstModuleView {
    AstModule const* data;
    AstNodes nodes;
} AstModuleView;

void debug_ast_module(AstModuleView v);
