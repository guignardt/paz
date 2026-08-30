#pragma once

#include "data/ast/identifier.h"

typedef struct AstPatternVariable {
    AstIdentifier identifier;
    struct AstExpression* type_name; // optional
} AstPatternVariable;

typedef enum AstPatternKind {
    AST_PATTERN_VARIABLE,
} AstPatternKind;

typedef struct AstPattern {
    // node
    struct AstNode* next;
    Range range;

    AstPatternKind kind;
    union {
        AstPatternVariable variable;
    } as;
} AstPattern;

void debug_ast_pattern(AstPattern const* v);
