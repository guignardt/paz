#include "data/ast.h"
#include "util/debug.h"

void debug_ast_module(AstModule v) {
    debug_begin("module");
    debug_attr_begin_list("bindings");
    for (AstBinding const* p = v.bindings; p; p = p->next) {
        debug_ast_binding(*p);
    }
    debug_attr_end_list();
    debug_end();
}

void debug_ast_program(AstProgram v) {
    debug_ast_module(v.root);
}

void debug_ast_binding(AstBinding v) {
    debug_begin("binding");
    debug_attr_int("kind", v.kind);
    if (v.pattern) {
        debug_attr_begin("pattern");
        debug_ast_pattern(*v.pattern);
        debug_attr_end();
    }
    if (v.value) {
        debug_attr_begin("value");
        debug_ast_expression(*v.value);
        debug_attr_end();
    }
    debug_end();
}

void debug_ast_expression(AstExpression v) {
    switch (v.kind) {
        case AST_EXPRESSION_BLOCK:
            debug_begin("block");
            debug_attr_begin_list("bindings");
            for (AstBinding const* p = v.as.block.bindings; p; p = p->next) {
                debug_ast_binding(*p);
            }
            debug_attr_end_list();
            if (v.as.block.tail) {
                debug_attr_begin("tail");
                debug_ast_expression(*v.as.block.tail);
                debug_attr_end();
            }
            debug_end();
            break;

        case AST_EXPRESSION_REF:
            debug_begin("ref");
            debug_attr_string("identifier", v.as.ref.identifier.string);
            debug_end();
            break;

        case AST_EXPRESSION_FUNCTION:
            debug_begin("function");
            if (v.as.function.input) {
                debug_attr_begin("input");
                debug_ast_pattern(*v.as.function.input);
                debug_attr_end();
            }
            if (v.as.function.output_type) {
                debug_attr_begin("output type");
                debug_ast_type_name(*v.as.function.output_type);
                debug_attr_end();
            }
            if (v.as.function.output) {
                debug_attr_begin("output");
                debug_ast_expression(*v.as.function.output);
                debug_attr_end();
            }
            debug_end();
            break;

        case AST_EXPRESSION_CONDITIONAL:
            debug_begin("conditional");
            if (v.as.conditional.condition) {
                debug_attr_begin("condition");
                debug_ast_expression(*v.as.conditional.condition);
                debug_attr_end();
            }
            if (v.as.conditional.if_true) {
                debug_attr_begin("if true");
                debug_ast_expression(*v.as.conditional.if_true);
                debug_attr_end();
            }
            if (v.as.conditional.if_false) {
                debug_attr_begin("if false");
                debug_ast_expression(*v.as.conditional.if_false);
                debug_attr_end();
            }
            debug_end();
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            debug_begin("unary operation");
            debug_attr_int("operator", v.as.unary_operation.operator);
            if (v.as.unary_operation.operand) {
                debug_attr_begin("operand");
                debug_ast_expression(*v.as.unary_operation.operand);
                debug_attr_end();
            }
            debug_end();
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            debug_begin("binary operation");
            debug_attr_int("operator", v.as.binary_operation.operator);
            if (v.as.binary_operation.lhs) {
                debug_attr_begin("lhs");
                debug_ast_expression(*v.as.binary_operation.lhs);
                debug_attr_end();
            }
            if (v.as.binary_operation.rhs) {
                debug_attr_begin("rhs");
                debug_ast_expression(*v.as.binary_operation.rhs);
                debug_attr_end();
            }
            debug_end();
            break;

        case AST_EXPRESSION_LITERAL_INT:
            debug_begin("int literal");
            debug_attr_int("value", v.as.literal_int);
            debug_end();
            break;
    }
}

void debug_ast_pattern(AstPattern v) {
    switch (v.kind) {
        case AST_PATTERN_VARIABLE:
            debug_begin("variable");
            debug_attr_string("identifier", v.as.variable.identifier.string);
            if (v.as.variable.type_name) {
                debug_attr_begin("type name");
                debug_ast_type_name(*v.as.variable.type_name);
                debug_attr_end();
            }
            debug_end();
            break;
    }
}

void debug_ast_type_name(AstTypeName v) {
    switch (v.kind) {
        case AST_TYPE_NAME_IDENTIFIER:
            debug_begin("identifier type name");
            debug_attr_string("identifier", v.as.identifier.string);
            debug_end();
            break;
    }
}
