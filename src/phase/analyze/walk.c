#include "phase/analyze.h"

static AnalyzeWalk analyze_walker_scope(AnalyzeWalk w, AnScopeId scope) {
    w._scope = scope;
    return w;
}

void analyze_walk_module(AnalyzeWalk w, AstModule const* p) {
    for (AstBinding* b = p->bindings; b; b = b->next) {
        analyze_walk_binding(w, b);
    }
}

void analyze_walk_expression(AnalyzeWalk w, AstExpression const* p) {
    AstExpression v = *p;

    AnScopeId id;
    AnalyzeWalk w2;

    switch (v.kind){
        case AST_EXPRESSION_BLOCK:;
            id = an_new_scope(w.scope_bindings, w._scope, false);
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
            id = an_new_scope(w.scope_bindings, w._scope, true);
            ptr_hash_table_insert(&w.expression_scopes->table, (intptr_t)p, &id);

            AnFunction an = {
                .expression = p,
                .captured_bindings = NULL,
                .captured_bindings_len = 0,
            };
            an_functions_register(w.functions, an);
            ptr_hash_table_insert(&w.function_ids, (intptr_t)p, &id);

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

void analyze_walk_type_name(AnalyzeWalk w, AstTypeName const* p) {
    AstTypeName v = *p;
    switch (v.kind) {
        case AST_TYPE_NAME_REFERENCE:
            break;
    }
}

void analyze_walk_pattern(AnalyzeWalk w, AstPattern const* p) {
    AstPattern v = *p;
    switch (v.kind) {
        case AST_PATTERN_VARIABLE:;
            AnBinding an = {
                .name = v.as.variable.identifier.string,
                .kind = w._binding_kind,
            };
            AnBindingId id = an_push_binding(w.scope_bindings, w._scope, an);
            ptr_hash_table_insert(&w.binding_ids->table, (intptr_t)p, &id);
            break;
    }
}

void analyze_walk_binding(AnalyzeWalk w, AstBinding const* p) {
    AstBinding v = *p;

    if (v.kind == AST_BINDING_CONSTANT) {
        an_register_constant_binding(w.constant_bindings, p);
    }

    analyze_walk_expression(w, v.value);

    switch (v.kind) {
        case AST_BINDING_CONSTANT: w._binding_kind = AN_BINDING_CONSTANT; break;
        case AST_BINDING_VARIABLE: w._binding_kind = AN_BINDING_VARIABLE; break;
    }

    analyze_walk_pattern(w, v.pattern);
}
