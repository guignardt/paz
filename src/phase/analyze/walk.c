#include "phase/analyze/analyze.h"

static AnalyzeWalker analyze_walker_scope(AnalyzeWalker w, AnScopeId scope) {
    w._scope = scope;
    return w;
}

void analyze_walk_module(AnalyzeWalker w, AstModule const* p) {
    for (AstBinding* b = p->bindings; b; b = b->next) {
        analyze_walk_binding(w, b);
    }
}

void analyze_walk_expression(AnalyzeWalker w, AstExpression const* p) {
    AstExpression v = *p;

    AnScopeId id;
    AnalyzeWalker w2;

    switch (v.kind){
        case AST_EXPRESSION_BLOCK:;
            id = an_new_scope(w.bindings, w._scope, false);
            ptr_hash_table_insert(&w.expression_scopes->table, (intptr_t)p, &id);
            
            w2 = analyze_walker_scope(w, id);
            for (AstBinding* b = v.as.block.bindings; b; b = b->next) {
                analyze_walk_binding(w2, b);
            }
            if (v.as.block.tail) {
                analyze_walk_expression(w2, v.as.block.tail);
            }
            break;

        case AST_EXPRESSION_REFERENCE:
            break;

        case AST_EXPRESSION_FUNCTION:;
            id = an_new_scope(w.bindings, w._scope, true);
            ptr_hash_table_insert(&w.expression_scopes->table, (intptr_t)p, &id);

            AnFunction an = {
                .expression = p,
                .captured_bindings = NULL,
                .captured_bindings_len = 0,
            };
            an_functions_register(w.functions, an);

            w2 = analyze_walker_scope(w, id);
            analyze_walk_pattern(w2, v.as.function.input);
            if (v.as.function.output_type) {
                analyze_walk_type_name(w2, v.as.function.output_type);
            }
            analyze_walk_expression(w2, v.as.function.output);
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            analyze_walk_expression(w, v.as.unary_operation.operand);
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            analyze_walk_expression(w, v.as.binary_operation.lhs);
            analyze_walk_expression(w, v.as.binary_operation.rhs);
            break;

        case AST_EXPRESSION_CONDITIONAL:
            analyze_walk_expression(w, v.as.conditional.condition);
            analyze_walk_expression(w, v.as.conditional.if_true);
            analyze_walk_expression(w, v.as.conditional.if_false);
            break;

        case AST_EXPRESSION_LITERAL_INT:
            break;
    }
}

void analyze_walk_type_name(AnalyzeWalker w, AstTypeName const* p) {
    AstTypeName v = *p;
    switch (v.kind) {
        case AST_TYPE_NAME_REFERENCE:
            break;
    }
}

void analyze_walk_pattern(AnalyzeWalker w, AstPattern const* p) {
    AstPattern v = *p;
    switch (v.kind) {
        case AST_PATTERN_VARIABLE:
            break;
    }
}

void analyze_walk_binding(AnalyzeWalker w, AstBinding const* p) {
    AstBinding v = *p;
    analyze_walk_pattern(w, v.pattern);
    analyze_walk_expression(w, v.value);
}
