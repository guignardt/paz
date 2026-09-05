#include "data/ast.h"

void ast_expression_visit(AstExpression const* v, AstExpressionVisitor visitor, void* data) {
    switch (v->kind) {
        case AST_EXPRESSION_BLOCK: visitor.block(&v->as.block, data); break;
        case AST_EXPRESSION_REFERENCE: visitor.reference(&v->as.reference, data); break;
        case AST_EXPRESSION_FUNCTION: visitor.function(&v->as.function, data); break;
        case AST_EXPRESSION_UNARY_OPERATION: visitor.unary_operation(&v->as.unary_operation, data); break;
        case AST_EXPRESSION_BINARY_OPERATION: visitor.binary_operation(&v->as.binary_operation, data); break;
        case AST_EXPRESSION_CONDITIONAL: visitor.conditional(&v->as.conditional, data); break;
        case AST_EXPRESSION_LITERAL_INT: visitor.literal_int(&v->as.literal_int, data); break;
    }
}
