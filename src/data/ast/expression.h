#pragma once

#include "data/ast/identifier.h"
#include "data/ast/pattern.h"

typedef struct AstBlock {
    struct AstBinding* bindings; // list
    struct AstExpression* tail; // optional
} AstBlock;

typedef struct AstRef {
    AstIdentifier identifier;
} AstRef;

typedef struct AstFunction {
    AstPattern* input;
    AstTypeName* output_type; // optional
    struct AstExpression* output;
} AstFunction;

typedef enum AstUnaryOperator {
    AST_OPERATOR_BREAK,
    AST_OPERATOR_CONTINUE,
} AstUnaryOperator;

typedef struct AstUnaryOperation {
    AstUnaryOperator operator;
    struct AstExpression* operand;
} AstUnaryOperation;

typedef enum AstBinaryOperator {
    AST_OPERATOR_CALL,

    AST_OPERATOR_ADD,

    AST_OPERATOR_FOR,
} AstBinaryOperator;

typedef struct AstBinaryOperation {
    AstBinaryOperator operator;
    struct AstExpression* lhs;
    struct AstExpression* rhs;
} AstBinaryOperation;

typedef struct AstFor {
    struct AstExpression* initial;
    struct AstExpression* transition;
} AstFor;

typedef struct AstBreak {
    struct AstExpression* value;
} AstBreak;

typedef struct AstContinue {
    struct AstExpression* value;
} AstContinue;

typedef enum AstExpressionKind {
    AST_EXPRESSION_BLOCK,
    AST_EXPRESSION_REF,
    AST_EXPRESSION_FUNCTION,
    AST_EXPRESSION_UNARY_OPERATION,
    AST_EXPRESSION_BINARY_OPERATION,
    AST_EXPRESSION_LITERAL_INT,
} AstExpressionKind;

typedef struct AstExpression {
    // node
    struct AstExpression* next;
    Range range;

    AstExpressionKind kind;
    union {
        AstBlock block;
        AstRef ref;
        AstFunction function;
        AstUnaryOperation unary_operation;
        AstBinaryOperation binary_operation;
        int64_t literal_int;
    } as;
} AstExpression;

void debug_ast_expression(AstExpression v);
