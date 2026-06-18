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
    AST_OPERATOR_CALL,

    AST_OPERATOR_ADD,
} AstBinaryOperator;

typedef struct AstBinaryOperation {
    AstBinaryOperator op;
    struct AstExpression* lhs;
    struct AstExpression* rhs;
} AstBinaryOperation;

typedef enum AstExpressionKind {
    AST_EXPRESSION_REF,
    AST_EXPRESSION_FUNCTION,
    AST_EXPRESSION_BINARY_OPERATION,
    AST_EXPRESSION_LITERAL_INT,
} AstExpressionKind;

typedef struct AstExpression {
    // node
    struct AstExpression* next;
    Range range;

    AstExpressionKind kind;
    union {
        AstRef ref;
        AstFunction function;
        AstBinaryOperation binary_operation;
        int64_t literal_int;
    } as;
} AstExpression;

void debug_ast_expression(AstExpression v);
