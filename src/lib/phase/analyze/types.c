#include "phase/analyze.h"
#include "diagnostic/report.h"

static void analyze_types_module(AstModuleView v, Analysis* a);
static void analyze_types_expression(AstExpressionView v, Analysis* a);
static void analyze_types_pattern(AstPatternView v, Analysis* a);
static void analyze_types_binding_set(AstBindingSetView v, Analysis* a);

static void analyze_types_block(AstExpressionView v, Analysis* a);
static void analyze_types_reference(AstExpressionView v, Analysis* a);
static void analyze_types_function(AstExpressionView v, Analysis* a);
static void analyze_types_unary_operation(AstExpressionView v, Analysis* a);
static void analyze_types_binary_operation(AstExpressionView v, Analysis* a);
static void analyze_types_conditional(AstExpressionView v, Analysis* a);
static void analyze_types_literal_int(AstExpressionView v, Analysis* a);

void analyze_types(Analysis* a) {
    switch (a->root.kind) {
        case AST_NODE_MODULE:
            analyze_types_module(a->root.as.module, a);
            break;
        case AST_NODE_EXPRESSION:
            analyze_types_expression(a->root.as.expression, a);
            break;
        case AST_NODE_PATTERN:
            analyze_types_pattern(a->root.as.pattern, a);
            break;
        case AST_NODE_BINDING_SET:
            analyze_types_binding_set(a->root.as.binding_set, a);
            break;
    }
}

static void analyze_types_module(AstModuleView v, Analysis* a) {
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        analyze_types_binding_set(b, a);
    }
}

static void analyze_types_expression(AstExpressionView v, Analysis* a) {
    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK: analyze_types_block(v, a); break;
        case AST_EXPRESSION_REFERENCE: analyze_types_reference(v, a); break;
        case AST_EXPRESSION_FUNCTION: analyze_types_function(v, a); break;
        case AST_EXPRESSION_UNARY_OPERATION: analyze_types_unary_operation(v, a); break;
        case AST_EXPRESSION_BINARY_OPERATION: analyze_types_binary_operation(v, a); break;
        case AST_EXPRESSION_CONDITIONAL: analyze_types_conditional(v, a); break;
        case AST_EXPRESSION_LITERAL_INT: analyze_types_literal_int(v, a); break;
    }
}

static void analyze_types_pattern(AstPatternView v, Analysis* a) {

}

static void analyze_types_binding_set(AstBindingSetView v, Analysis* a) {

}

static void analyze_types_block(AstExpressionView v, Analysis* a) {
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        analyze_types_binding_set(b, a);
    }
    AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
    if (tail.data) {
        analyze_types_expression(tail, a);
        a->expression_types[v.id] = a->expression_types[tail.id];
    } else {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("block has no tail");
        report_end();
    }
}

static void analyze_types_reference(AstExpressionView v, Analysis* a) {
    AnBindingId binding_id = *(AnBindingId const*)id_hash_table_get(a->variable_bindings, v.id);
    AnBinding binding = *an_get_binding(a->scope_bindings, binding_id);
    // type is resolved at this point: if `const`, it was resolved in
    // the previous subphase, and if `let`, it was seen before in the AST.
    a->expression_types[v.id] = binding.type;
}

static void analyze_types_function(AstExpressionView v, Analysis* a) {

}

static void analyze_types_unary_operation(AstExpressionView v, Analysis* a);
static void analyze_types_binary_operation(AstExpressionView v, Analysis* a);
static void analyze_types_literal_int(AstExpressionView v, Analysis* a);
