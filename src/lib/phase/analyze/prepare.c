#include "phase/analyze.h"

#include "diagnostic/report.h"

Analyzer analyzer_init(Analysis* a) {
    return (Analyzer) {
        .analysis = a,
        ._scope_location = { .scope = ID_SENTINEL },
        ._binding_kind = AN_BINDING_CONSTANT,
        ._binding_set = ID_SENTINEL,
    };
}

void analyze_prepare_module(Analyzer w, AstModuleView v) {
    AnScopeId scope_id = an_new_scope(
        &w.analysis->scope_bindings,
        (AnScopeLocation) { .scope = ID_SENTINEL },
        false
    );
    
    AnBinding int_binding = {
        .name = (String) { .data = "Int", .len = 3 },
        .kind = AN_BINDING_CONSTANT,
        .as.constant = (AnConstant) {
            .kind = AN_CONSTANT_TYPE,
            .as.type = AN_TYPE_ID_INT,
            .type = AN_TYPE_ID_TYPE,
        },
        .source_binding_set = ID_SENTINEL,
        .type = AN_TYPE_ID_TYPE,
    };
    an_add_binding(&w.analysis->scope_bindings, scope_id, int_binding);

    w._scope_location = an_scope_constant_location(scope_id);
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        analyze_prepare_binding_set(w, b);
    }
}

void analyze_prepare_expression(Analyzer w, AstExpressionView v, bool explicitly_constant) {
    if (explicitly_constant) {
        // this expression and children can't reference a `let` binding from outside
        w._scope_location = an_scope_constant_location(w._scope_location.scope);

        AnConstantItem constant_item = {
            .kind = AN_CONSTANT_ITEM_EXPRESSION,
            .id = v.id,
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
    AstExpressionView output_type;
    AnBindingLocation binding_location;
    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK:
            scope_id = an_new_scope(&w.analysis->scope_bindings, w._scope_location, false);
            w._scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);

            for (
                AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
                b.data;
                b = AST_VIEW_CHILD_BINDING_SET(b, .next)
            ) {
                analyze_prepare_binding_set(w, b);
                // FIXME: is this the right place for this?
                w._scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);
            }

            AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
            if (tail.data) {
                analyze_prepare_expression(w, tail, false);
            }

            break;

        case AST_EXPRESSION_REFERENCE:
            binding_location = (AnBindingLocation) {
                .location = w._scope_location,
                .binding = {0} // filled in during binding resolution
            };
            id_hash_table_insert(
                &w.analysis->reference_binding_locations.table,
                v.id,
                &binding_location
            );
            break;

        case AST_EXPRESSION_FUNCTION:
            function = (AnFunction) { .expression = v.id };
            function_id = an_functions_register(&w.analysis->functions, function);
            id_hash_table_insert(&w.analysis->function_ids.table, v.id, &function_id);

            output_type = AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type);
            if (output_type.data) {
                analyze_prepare_expression(w, output_type, true);
            }

            // TODO: allow captures
            // For now, we only allow a function body to access constants from the parent scope.
            // This effectively prevents captures.
            scope_id = an_new_scope(
                &w.analysis->scope_bindings,
                an_scope_constant_location(w._scope_location.scope),
                true
            );
            w._scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);

            w._binding_kind = AN_BINDING_VARIABLE;
            w._binding_set = ID_SENTINEL;
            analyze_prepare_pattern(w, AST_VIEW_CHILD_PATTERN(v, .as.function.input));
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.function.output), false);

            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.unary_operation.operand), false);
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.lhs), false);
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.rhs), false);
            break;
            
        case AST_EXPRESSION_CONDITIONAL:
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.condition), false);
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_true), false);
            analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_false), false);
            break;

        case AST_EXPRESSION_LITERAL_INT:
            break;
    }
}

void analyze_prepare_pattern(Analyzer w, AstPatternView v) {
    String name;
    AnBinding binding;
    AnBindingId binding_id;
    AstExpressionView type_bound;
    switch (v.data->kind) {
        case AST_PATTERN_VARIABLE:
            name = v.data->as.variable.identifier.string;
            if (w._binding_kind == AN_BINDING_CONSTANT) {
                if (!an_find_binding_in_scope(
                    w.analysis->scope_bindings,
                    // FIXME: is this right?
                    an_scope_end_location(w.analysis->scope_bindings, w._scope_location.scope),
                    name,
                    NULL
                )) {
                    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                    report_message("duplicate binding `%.*s`", (int)name.len, name.data);
                    report_end();
                    break;
                }
            }

            binding = (AnBinding) {
                .name = name,
                .kind = w._binding_kind,
                .source_binding_set = w._binding_set
            };
            binding_id = an_add_binding(&w.analysis->scope_bindings, w._scope_location.scope, binding);
            id_hash_table_insert(&w.analysis->variable_bindings.table, v.id, &binding_id);

            type_bound = AST_VIEW_CHILD_EXPRESSION(v, .as.variable.type_bound);
            if (type_bound.data) {
                analyze_prepare_expression(w, type_bound, true);
            }

            break;
    }
}

void analyze_prepare_binding_set(Analyzer w, AstBindingSetView v) {
    switch (v.data->kind) {
        case AST_BINDING_SET_CONSTANT: w._binding_kind = AN_BINDING_CONSTANT; break;
        case AST_BINDING_SET_VARIABLE: w._binding_kind = AN_BINDING_VARIABLE; break;
    }

    if (v.data->kind == AST_BINDING_SET_CONSTANT) {
        AnConstantItem item = {
            .kind = AN_CONSTANT_ITEM_BINDING,
            .id = v.id,
        };
        reserve(
            ERASE2(&w.analysis->constant_items.data),
            &w.analysis->constant_items._capacity,
            w.analysis->constant_items.len + 1,
            sizeof(AnConstantItem)
        );
        w.analysis->constant_items.data[w.analysis->constant_items.len++] = item;
    }

    w._binding_set = v.id;
    analyze_prepare_pattern(w, AST_VIEW_CHILD_PATTERN(v, .pattern));
    analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .value), v.data->kind == AST_BINDING_SET_CONSTANT);
}
