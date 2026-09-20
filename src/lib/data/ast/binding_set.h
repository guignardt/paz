#pragma once

#include "data/ast/nodes.h"
#include "data/ast/identifier.h"

typedef enum AstBindingSetKind {
    AST_BINDING_SET_CONSTANT,
    AST_BINDING_SET_VARIABLE,
} AstBindingSetKind;

typedef struct AstBindingSet {
    // node
    AstBindingSetId next;
    Range range;

    AstBindingSetKind kind;
    AstPatternId pattern;
    AstExpressionId value;
} AstBindingSet;

typedef struct AstBindingSetView {
    AstBindingSet const* data;
    AstBindingSetId id;
    AstNodes nodes;
} AstBindingSetView;

#define AST_VIEW_BINDING_SET(n, i)                                              \
    ((AstBindingSetView) {                                                      \
        .data = ((i) == ID_SENTINEL ? NULL : &(n).binding_sets.data[i]),        \
        .id = (i),                                                              \
        .nodes = (n),                                                           \
    })

#define AST_VIEW_CHILD_BINDING_SET(v, f)                                        \
    AST_VIEW_BINDING_SET((v).nodes, (*v.data)f)

void debug_ast_binding_set(AstBindingSetView v);
