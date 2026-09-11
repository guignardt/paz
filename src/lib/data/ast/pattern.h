#pragma once

#include "data/ast/identifier.h"

typedef struct AstPatternVariable {
    AstIdentifier identifier;
    AstExpressionId type_bound; // optional
} AstPatternVariable;

typedef enum AstPatternKind {
    AST_PATTERN_VARIABLE,
} AstPatternKind;

typedef struct AstPattern {
    // node
    AstPatternId next;
    Range range;

    AstPatternKind kind;
    union {
        AstPatternVariable variable;
    } as;
} AstPattern;

typedef struct AstPatternView {
    AstPattern const* data;
    AstPatternId id;
    AstNodes nodes;
} AstPatternView;

#define AST_VIEW_PATTERN(n, i)                                  \
    ((AstPatternView) {                                         \
        .data = ((i) == ID_SENTINEL ? NULL : &(n).patterns[i]), \
        .id = (i),                                              \
        .nodes = (n),                                           \
    })

#define AST_VIEW_CHILD_PATTERN(v, f)                            \
    AST_VIEW_PATTERN((v).nodes, (*v.data)f)

void debug_ast_pattern(AstPatternView v);
