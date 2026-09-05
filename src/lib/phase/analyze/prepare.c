#include "phase/analyze.h"

#include "diagnostic/report.h"

Analyzer analyzer_init(Analysis* a) {
    return (Analyzer) {
        .analysis = a,
        ._scope = AN_SCOPE_ID_SENTINEL,
        ._binding_kind = AN_BINDING_CONSTANT,
        ._binding = NULL,
    };
}

void analyze_prepare_module(Analyzer w, AstModule const* v) {
    AnScopeId scope_id = an_new_scope(&w.analysis->scope_bindings, AN_SCOPE_ID_SENTINEL, false);
    w._scope = scope_id;
    for (AstBinding* p = v->bindings; p; p = p->next) {
        analyze_prepare_binding(w, p);
    }
}

void analyze_prepare_expression(Analyzer w, AstExpression const* v, bool explicitly_constant) {
    if (explicitly_constant) {
        AnConstantItem constant_item = {
            .kind = AN_CONSTANT_ITEM_EXPRESSION,
            .as.expression = v,
        };
        reserve(
            ERASE2(&w.analysis->constant_items.data),
            &w.analysis->constant_items._capacity,
            w.analysis->constant_items.len + 1,
            sizeof(AnConstantItem)
        );
        w.analysis->constant_items.data[w.analysis->constant_items.len++] = constant_item;
    }

    AnScopeId scope_id;
    AnFunction function;
    AnFunctionId function_id;
    switch (v->kind) {
        case AST_EXPRESSION_BLOCK:
            scope_id = an_new_scope(&w.analysis->scope_bindings, w._scope, false);
            w._scope = scope_id;

            for (AstBinding* b = v->as.block.bindings; b; b = b->next) {
                analyze_prepare_binding(w, b);
            }

            if (v->as.block.tail) {
                analyze_prepare_expression(w, v->as.block.tail, false);
            }

            break;

        case AST_EXPRESSION_REFERENCE:
            ptr_hash_table_insert(&w.analysis->reference_scopes.table, (intptr_t)v, &w._scope);
            break;

        case AST_EXPRESSION_FUNCTION:
            function = (AnFunction) { .expression = v };
            function_id = an_functions_register(&w.analysis->functions, function);
            ptr_hash_table_insert(&w.analysis->function_ids.table, (intptr_t)v, &function_id);

            if (v->as.function.output_type) {
                analyze_prepare_expression(w, v->as.function.output_type, true);
            }

            scope_id = an_new_scope(&w.analysis->scope_bindings, AN_SCOPE_ID_SENTINEL, false);
            w._scope = scope_id;

            w._binding_kind = AN_BINDING_VARIABLE;
            w._binding = NULL;
            analyze_prepare_pattern(w, v->as.function.input);
            analyze_prepare_expression(w, v->as.function.output, false);

            break;
            
        default: break;
    }
}

void analyze_prepare_pattern(Analyzer w, AstPattern const* v) {
    if (w._binding_kind != AN_BINDING_CONSTANT) {
        return;
    }

    String name;
    AnBinding binding;
    AnBindingId binding_id;
    switch (v->kind) {
        case AST_PATTERN_VARIABLE:
            name = v->as.variable.identifier.string;
            
            if (!an_find_binding_in_scope(w.analysis->scope_bindings, w._scope, name, NULL)) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("duplicate binding `%.*s`", (int)name.len, name.data);
                report_end();
                break;
            }

            binding = (AnBinding) {
                .name = name,
                .kind = AN_BINDING_CONSTANT,
                .source_binding = w._binding
            };
            binding_id = an_push_binding(&w.analysis->scope_bindings, w._scope, binding);
            ptr_hash_table_insert(&w.analysis->binding_ids.table, (intptr_t)v, &binding_id);

            if (v->as.variable.type_bound) {
                analyze_prepare_expression(w, v->as.variable.type_bound, true);
            }

            break;
    }

    AnConstantItem constant_item = {
        .kind = AN_CONSTANT_ITEM_BINDING,
        .as.binding = w._binding,
    };
    reserve(
        ERASE2(&w.analysis->constant_items.data),
        &w.analysis->constant_items._capacity,
        w.analysis->constant_items.len + 1,
        sizeof(AnConstantItem)
    );
    w.analysis->constant_items.data[w.analysis->constant_items.len++] = constant_item;
}

void analyze_prepare_binding(Analyzer w, AstBinding const* v) {
    switch (v->kind) {
        case AST_BINDING_CONSTANT: w._binding_kind = AN_BINDING_CONSTANT; break;
        case AST_BINDING_VARIABLE: w._binding_kind = AN_BINDING_VARIABLE; break;
    }

    analyze_prepare_pattern(w, v->pattern);
    analyze_prepare_expression(w, v->value, false);
}
