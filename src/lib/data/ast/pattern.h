#pragma once

#include "data/ast/identifier.h"

typedef struct AstPatternVariable {
    AstIdentifier identifier;
} AstPatternVariable;

typedef struct AstPatternTuple {
    AstPatternId elements; // list
} AstPatternTuple;

typedef enum AstPatternKind {
    AST_PATTERN_VARIABLE,
    AST_PATTERN_TUPLE,
} AstPatternKind;

typedef struct AstPattern {
    // node
    AstPatternId next;
    Range range;

    AstPatternKind kind;
    union {
        AstPatternVariable variable;
        AstPatternTuple tuple;
    } as;
    AstExpressionId type_bound; // optional
} AstPattern;

typedef struct AstPatternView {
    AstPattern const* data;
    AstPatternId id;
    AstNodes nodes;
} AstPatternView;

#define AST_VIEW_PATTERN(n, i)                                          \
    ((AstPatternView) {                                                 \
        .data = ((i) == ID_SENTINEL ? NULL : &(n).patterns.data[i]),    \
        .id = (i),                                                      \
        .nodes = (n),                                                   \
    })

#define AST_VIEW_CHILD_PATTERN(v, f)                            \
    AST_VIEW_PATTERN((v).nodes, (*v.data)f)

void debug_ast_pattern(AstPatternView v);
