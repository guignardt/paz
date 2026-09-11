#pragma once

#include "data/ast/nodes.h"
#include "data/ast/identifier.h"

typedef struct AstBlock {
    AstBindingSetId binding_sets; // list
    AstExpressionId tail; // optional
} AstBlock;

typedef struct AstReference {
    AstIdentifier identifier;
} AstReference;

typedef struct AstFunction {
    AstPatternId input;
    AstExpressionId output_type; // optional
    AstExpressionId output;
} AstFunction;

typedef enum AstUnaryOperator {
    AST_OPERATOR_BREAK,
    AST_OPERATOR_CONTINUE,
} AstUnaryOperator;

typedef struct AstUnaryOperation {
    AstUnaryOperator operator;
    AstExpressionId operand;
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
} AstBinaryOperator;

typedef struct AstBinaryOperation {
    AstBinaryOperator operator;
    AstExpressionId lhs;
    AstExpressionId rhs;
} AstBinaryOperation;

typedef struct AstConditional {
    AstExpressionId condition;
    AstExpressionId if_true;
    AstExpressionId if_false;
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
    AstExpressionId next;
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

typedef struct AstExpressionView {
    AstExpression const* data;
    AstExpressionId id;
    AstNodes nodes;
} AstExpressionView;

#define AST_VIEW_EXPRESSION(n, i)                                   \
    ((AstExpressionView) {                                          \
        .data = ((i) == ID_SENTINEL ? NULL : &(n).expressions[i]),  \
        .id = (i),                                                  \
        .nodes = (n),                                               \
    })

#define AST_VIEW_CHILD_EXPRESSION(v, f)                             \
    AST_VIEW_EXPRESSION((v).nodes, (*v.data)f)

void debug_ast_expression(AstExpressionView v);
