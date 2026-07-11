#pragma once

#include "data/ast/identifier.h"
#include "data/ast/pattern.h"

typedef struct AstBlock {
    struct AstBinding* bindings; // list
    struct AstExpression* tail; // optional
} AstBlock;

typedef struct AstReference {
    AstIdentifier identifier;
} AstReference;

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

    AST_OPERATOR_EQ,
    AST_OPERATOR_NE,
    AST_OPERATOR_LE,
    AST_OPERATOR_LT,
    AST_OPERATOR_GE,
    AST_OPERATOR_GT,

    AST_OPERATOR_ADD,
    AST_OPERATOR_SUB,
    AST_OPERATOR_MUL,
    AST_OPERATOR_DIV,
    AST_OPERATOR_REM,

    AST_OPERATOR_FOR,
} AstBinaryOperator;

typedef struct AstBinaryOperation {
    AstBinaryOperator operator;
    struct AstExpression* lhs;
    struct AstExpression* rhs;
} AstBinaryOperation;

typedef struct AstConditional {
    struct AstExpression* condition;
    struct AstExpression* if_true;
    struct AstExpression* if_false;
} AstConditional;

typedef enum AstExpressionKind {
    AST_EXPRESSION_BLOCK,
    AST_EXPRESSION_REFERENCE,
    AST_EXPRESSION_FUNCTION,
    AST_EXPRESSION_UNARY_OPERATION,
    AST_EXPRESSION_BINARY_OPERATION,
    AST_EXPRESSION_CONDITIONAL,
    AST_EXPRESSION_LITERAL_INT,
} AstExpressionKind;

typedef struct AstExpression {
    // node
    struct AstExpression* next;
    Range range;

    AstExpressionKind kind;
    union {
        AstBlock block;
        AstReference reference;
        AstFunction function;
        AstUnaryOperation unary_operation;
        AstBinaryOperation binary_operation;
        AstConditional conditional;
        int64_t literal_int;
    } as;
} AstExpression;

typedef struct AstExpressionVisitor {
    void(*block)            (AstBlock const* v,             void* data);
    void(*reference)        (AstReference const* v,         void* data);
    void(*function)         (AstFunction const* v,          void* data);
    void(*unary_operation)  (AstUnaryOperation const* v,    void* data);
    void(*binary_operation) (AstBinaryOperation const* v,   void* data);
    void(*conditional)      (AstConditional const* v,       void* data);
    void(*literal_int)      (int64_t const* v,              void* data);
} AstExpressionVisitor;

void ast_expression_visit(AstExpression const* v, AstExpressionVisitor visitor, void* data);

void debug_ast_expression(AstExpression const* v);
