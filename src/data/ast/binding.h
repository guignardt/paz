#pragma once

#include "data/ast/identifier.h"

typedef enum AstBindingKind {
    AST_BINDING_CONSTANT,
    AST_BINDING_VARIABLE,
} AstBindingKind;

typedef struct AstBinding {
    // node
    struct AstBinding* next;
    Range range;

    AstBindingKind kind;
    struct AstPattern* pattern;
    struct AstExpression* value;
} AstBinding;

void debug_ast_binding(AstBinding v);
