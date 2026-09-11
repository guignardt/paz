#include "data/ast.h"
#include "util/debug.h"

void debug_ast_module(AstModuleView p) {
    if (!p.data) return;

    debug_begin("module");

    debug_attr_begin_list("binding sets");
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(p, .binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        debug_ast_binding_set(b);
    }
    debug_attr_end_list();

    debug_end();
}

void debug_ast_binding_set(AstBindingSetView p) {
    if (!p.data) return;
    AstBindingSet v = *p.data;

    debug_begin("binding set");
    debug_attr_int("id", p.id);

    debug_attr_int("kind", v.kind);

    debug_attr_begin("pattern");
    debug_ast_pattern(AST_VIEW_CHILD_PATTERN(p, .pattern));
    debug_attr_end();
    
    debug_attr_begin("value");
    debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .value));
    debug_attr_end();

    debug_end();
}

void debug_ast_expression(AstExpressionView p) {
    if (!p.data) return;
    AstExpression v = *p.data;

    switch (v.kind) {
        case AST_EXPRESSION_BLOCK:
            debug_begin("block");
            debug_attr_int("id", p.id);

            debug_attr_begin_list("bindings");
            for (
                AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(p, .as.block.binding_sets);
                b.data;
                b = AST_VIEW_CHILD_BINDING_SET(b, .next)
            ) {
                debug_ast_binding_set(b);
            }
            debug_attr_end_list();

            debug_attr_begin("tail");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.block.tail));
            debug_attr_end();

            debug_end();
            break;

        case AST_EXPRESSION_REFERENCE:
            debug_begin("reference");
            debug_attr_int("id", p.id);
            debug_attr_string("identifier", v.as.reference.identifier.string);
            debug_end();
            break;

        case AST_EXPRESSION_FUNCTION:
            debug_begin("function");
            debug_attr_int("id", p.id);

            debug_attr_begin("input");
            debug_ast_pattern(AST_VIEW_CHILD_PATTERN(p, .as.function.input));
            debug_attr_end();

            debug_attr_begin("output type");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.function.output_type));
            debug_attr_end();

            debug_attr_begin("output");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.function.output));
            debug_attr_end();

            debug_end();
            break;

        case AST_EXPRESSION_CONDITIONAL:
            debug_begin("conditional");
            debug_attr_int("id", p.id);

            debug_attr_begin("condition");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.conditional.condition));
            debug_attr_end();

            debug_attr_begin("if true");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.conditional.if_true));
            debug_attr_end();

            debug_attr_begin("if false");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.conditional.if_false));
            debug_attr_end();

            debug_end();
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            debug_begin("unary operation");
            debug_attr_int("id", p.id);

            debug_attr_int("operator", v.as.unary_operation.operator);

            debug_attr_begin("operand");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.unary_operation.operand));
            debug_attr_end();

            debug_end();
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            debug_begin("binary operation");
            debug_attr_int("id", p.id);

            debug_attr_int("operator", v.as.binary_operation.operator);
            
            debug_attr_begin("lhs");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.binary_operation.lhs));
            debug_attr_end();

            debug_attr_begin("rhs");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.binary_operation.rhs));
            debug_attr_end();

            debug_end();
            break;

        case AST_EXPRESSION_LITERAL_INT:
            debug_begin("int literal");
            debug_attr_int("id", p.id);
            debug_attr_int("value", v.as.literal_int);
            debug_end();
            break;
    }
}

void debug_ast_pattern(AstPatternView p) {
    if (!p.data) return;
    AstPattern v = *p.data;

    switch (v.kind) {
        case AST_PATTERN_VARIABLE:
            debug_begin("variable");
            debug_attr_int("id", p.id);
            
            debug_attr_string("identifier", v.as.variable.identifier.string);

            debug_attr_begin("type name");
            debug_ast_expression(AST_VIEW_CHILD_EXPRESSION(p, .as.variable.type_bound));
            debug_attr_end();

            debug_end();
            break;
    }
}
