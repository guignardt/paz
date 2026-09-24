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

typedef enum AstNodeKind {
    AST_NODE_MODULE,
    AST_NODE_EXPRESSION,
    AST_NODE_PATTERN,
    AST_NODE_BINDING_SET,
} AstNodeKind;

typedef struct AstNodeView {
    AstNodeKind kind;
    union {
        AstModuleView module;
        AstExpressionView expression;
        AstPatternView pattern;
        AstBindingSetView binding_set;
    } as;
} AstNodeView;

AstNodes ast_node_view_nodes(AstNodeView v);
