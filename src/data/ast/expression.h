#pragma once

#include "data/ast/identifier.h"
#include "data/ast/pattern.h"

typedef struct AstRef {
    AstIdentifier identifier;
} AstRef;

typedef struct AstFunction {
    AstPattern* input;
    AstTypeName* output_type; // optional
    struct AstExpression* output;
} AstFunction;

typedef enum AstBinaryOperator {
    AST_OPERATOR_ADD,
    AST_OPERATOR_SUB,
    AST_OPERATOR_MUL,
    AST_OPERATOR_DIV,
    AST_OPERATOR_REM,
} AstBinaryOperator;

typedef struct AstBinaryOperation {
    AstBinaryOperator op;
    struct AstExpression* first;
    struct AstExpression* second;
} AstBinaryOperation;

typedef enum AstExpressionKind {
    AST_EXPRESSION_REF,
    // AST_EXPRESSION_BINARY_OPERATION,
    AST_EXPRESSION_FUNCTION,
} AstExpressionKind;

typedef struct AstExpression {
    // node
    struct AstExpression* next;
    Range range;

    AstExpressionKind kind;
    union {
        AstRef ref;
        AstFunction function;
    } as;
} AstExpression;

void debug_ast_expression(AstExpression v);
