#include "phase/analyze.h"

AnalyzeWalk analyze_walk_init(Analysis* analysis) {
    return (AnalyzeWalk) {
        .analysis = analysis,
        ._scope = AN_SCOPE_ID_SENTINEL,
        ._binding_kind = AN_BINDING_CONSTANT,
    };
}

static AnalyzeWalk analyze_walk_scope(AnalyzeWalk w, AnScopeId scope) {
    w._scope = scope;
    return w;
}

void analyze_walk_const_module(AnalyzeWalk w, AstModule const* p) {
    w._scope = an_new_scope(&w.analysis->scope_bindings, AN_SCOPE_ID_SENTINEL, false);
    for (AstBinding* b = p->bindings; b; b = b->next) {
        analyze_walk_const_binding(w, b);
    }
}

void analyze_walk_const_expression(AnalyzeWalk w, AstExpression const* p) {
    AstExpression v = *p;

    AnScopeId scope_id;
    AnalyzeWalk w2;
    AnFunctionId function_id;

    switch (v.kind) {
        case AST_EXPRESSION_BLOCK:
            scope_id = an_new_scope(&w.analysis->scope_bindings, w._scope, false);
            ptr_hash_table_insert(&w.analysis->expression_scopes.table, (intptr_t)p, &scope_id);
            
            w2 = analyze_walk_scope(w, scope_id);
            for (AstBinding* b = v.as.block.bindings; b; b = b->next) {
                analyze_walk_const_binding(w2, b);
            }
            if (v.as.block.tail) {
                analyze_walk_const_expression(w2, v.as.block.tail);
            }

            break;

        case AST_EXPRESSION_REFERENCE:
            break;

        case AST_EXPRESSION_FUNCTION:
            scope_id = an_new_scope(&w.analysis->scope_bindings, w._scope, false);
            ptr_hash_table_insert(&w.analysis->expression_scopes.table, (intptr_t)p, &scope_id);
            
            AnFunction an = {
                .expression = p,
                .captured_bindings = NULL,
                .captured_bindings_len = 0,
            };
            function_id = an_functions_register(&w.analysis->functions, an);
            ptr_hash_table_insert(&w.analysis->function_ids.table, (intptr_t)p, &function_id);

            w2 = analyze_walk_scope(w, scope_id);
            analyze_walk_const_pattern(w2, v.as.function.input);
            if (v.as.function.output_type) {
                analyze_walk_const_type_name(w2, v.as.function.output_type);
            }
            analyze_walk_const_expression(w2, v.as.function.output);
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            analyze_walk_const_expression(w, v.as.unary_operation.operand);
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            analyze_walk_const_expression(w, v.as.binary_operation.lhs);
            analyze_walk_const_expression(w, v.as.binary_operation.rhs);
            break;

        case AST_EXPRESSION_CONDITIONAL:
            analyze_walk_const_expression(w, v.as.conditional.condition);
            analyze_walk_const_expression(w, v.as.conditional.if_true);
            analyze_walk_const_expression(w, v.as.conditional.if_false);
            break;

        case AST_EXPRESSION_LITERAL_INT:
            break;

    }
}

void analyze_walk_const_type_name(AnalyzeWalk w, AstTypeName const* p) {
    AstTypeName v = *p;
    switch (v.kind) {
        case AST_TYPE_NAME_REFERENCE:
            break;
    }
}

void analyze_walk_const_pattern(AnalyzeWalk w, AstPattern const* p) {
    AstPattern v = *p;
    switch (v.kind) {
        case AST_PATTERN_VARIABLE:;
            if (w._binding_kind == AN_BINDING_CONSTANT) {
                AnBinding an = {
                    .name = v.as.variable.identifier.string,
                    .kind = w._binding_kind,
                };
                AnBindingId id = an_push_binding(&w.analysis->scope_bindings, w._scope, an);
                ptr_hash_table_insert(&w.analysis->binding_ids.table, (intptr_t)p, &id);
            }
            break;
    }
}

void analyze_walk_const_binding(AnalyzeWalk w, AstBinding const* p) {
    AstBinding v = *p;

    if (v.kind == AST_BINDING_CONSTANT) {
        an_register_constant_binding(&w.analysis->constant_bindings, p);
    }

    analyze_walk_const_expression(w, v.value);

    if (v.kind == AST_BINDING_CONSTANT) {
        w._binding_kind = AN_BINDING_CONSTANT;
        analyze_walk_const_pattern(w, v.pattern);
    }
}
