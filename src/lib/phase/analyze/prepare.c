#include "phase/analyze.h"

#include "diagnostic/report.h"

typedef struct AnPreparer {
    Analysis* analysis;
    AnScopeLocation scope_location;
    AnBindingKind binding_kind;
    AstBindingSetId binding_set;
} AnPreparer;

static void analyze_prepare_module(AnPreparer w, AstModuleView v);
static void analyze_prepare_expression(AnPreparer w, AstExpressionView v, bool consteval);
static void analyze_prepare_pattern(AnPreparer w, AstPatternView v);
static void analyze_prepare_binding_set(AnPreparer w, AstBindingSetView v);

void analyze_prepare(Analysis* a, bool consteval) {
    free(a->expression_types);
    a->expression_types = alloc_or_exit(ast_node_view_nodes(a->root).expressions.len * sizeof(AnTypeId));

    AnPreparer w = {
        .analysis = a,
        .scope_location = { .scope = ID_SENTINEL },
        .binding_kind = AN_BINDING_CONSTANT,
        .binding_set = ID_SENTINEL,
    };
    switch (a->root.kind) {
        case AST_NODE_MODULE:
            analyze_prepare_module(w, a->root.as.module);
            break;
        case AST_NODE_EXPRESSION:
            analyze_prepare_expression(w, a->root.as.expression, consteval);
            break;
        case AST_NODE_PATTERN:
            analyze_prepare_pattern(w, a->root.as.pattern);
            break;
        case AST_NODE_BINDING_SET:
            analyze_prepare_binding_set(w, a->root.as.binding_set);
            break;
    }
}

void analyze_prepare_module(AnPreparer w, AstModuleView v) {
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

    w.scope_location = an_scope_constant_location(scope_id);
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        analyze_prepare_binding_set(w, b);
    }
}

void analyze_prepare_expression(AnPreparer w, AstExpressionView v, bool consteval) {
    if (consteval) {
        // this expression and children can't reference a `let` binding from outside
        w.scope_location = an_scope_constant_location(w.scope_location.scope);

        AnConstantItem constant_item = {
            .kind = AN_CONSTANT_ITEM_EXPRESSION,
            .id = v.id,
        };
        DYNAMIC_ARRAY_PUSH(&w.analysis->constant_items, constant_item);
    }

    AnScopeId scope_id;
    AnFunction function;
    AnFunctionId function_id;
    AstExpressionView output_type;
    AnBindingLocation binding_location;
    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK:
            scope_id = an_new_scope(&w.analysis->scope_bindings, w.scope_location, false);
            w.scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);

            for (
                AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
                b.data;
                b = AST_VIEW_CHILD_BINDING_SET(b, .next)
            ) {
                analyze_prepare_binding_set(w, b);
                // FIXME: is this the right place for this?
                w.scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);
            }

            AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
            if (tail.data) {
                analyze_prepare_expression(w, tail, false);
            }

            break;

        case AST_EXPRESSION_REFERENCE:
            binding_location = (AnBindingLocation) {
                .location = w.scope_location,
                .binding = {0} // filled in during binding resolution
            };
            id_hash_table_insert(
                &w.analysis->reference_binding_locations,
                v.id,
                &binding_location
            );
            break;

        case AST_EXPRESSION_FUNCTION:
            function = (AnFunction) { .expression = v.id };
            function_id = an_functions_register(&w.analysis->functions, function);
            id_hash_table_insert(&w.analysis->function_ids, v.id, &function_id);

            output_type = AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type);
            if (output_type.data) {
                analyze_prepare_expression(w, output_type, true);
            }

            // TODO: allow captures
            // For now, we only allow a function body to access constants from the parent scope.
            // This effectively prevents captures.
            scope_id = an_new_scope(
                &w.analysis->scope_bindings,
                an_scope_constant_location(w.scope_location.scope),
                true
            );
            w.scope_location = an_scope_end_location(w.analysis->scope_bindings, scope_id);

            w.binding_kind = AN_BINDING_VARIABLE;
            w.binding_set = ID_SENTINEL;
            analyze_prepare_pattern(w, AST_VIEW_CHILD_PATTERN(v, .as.function.input));
            // FIXME: this probably isn't the right place for setting the scope location.
            w.scope_location = an_scope_end_location(w.analysis->scope_bindings, w.scope_location.scope);
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

void analyze_prepare_pattern(AnPreparer w, AstPatternView v) {
    String name;
    AnBinding binding;
    AnBindingId binding_id;
    AstExpressionView type_bound;
    switch (v.data->kind) {
        case AST_PATTERN_VARIABLE:
            name = v.data->as.variable.identifier.string;
            if (w.binding_kind == AN_BINDING_CONSTANT) {
                if (!an_find_binding_in_scope(
                    w.analysis->scope_bindings,
                    // FIXME: is this right?
                    an_scope_constant_location(w.scope_location.scope),
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
                .kind = w.binding_kind,
                .source_binding_set = w.binding_set
            };
            binding_id = an_add_binding(&w.analysis->scope_bindings, w.scope_location.scope, binding);
            id_hash_table_insert(&w.analysis->variable_bindings, v.id, &binding_id);

            type_bound = AST_VIEW_CHILD_EXPRESSION(v, .as.variable.type_bound);
            if (type_bound.data) {
                analyze_prepare_expression(w, type_bound, true);
            }

            break;
    }
}

void analyze_prepare_binding_set(AnPreparer w, AstBindingSetView v) {
    switch (v.data->kind) {
        case AST_BINDING_SET_CONSTANT: w.binding_kind = AN_BINDING_CONSTANT; break;
        case AST_BINDING_SET_VARIABLE: w.binding_kind = AN_BINDING_VARIABLE; break;
    }

    if (v.data->kind == AST_BINDING_SET_CONSTANT) {
        AnConstantItem item = {
            .kind = AN_CONSTANT_ITEM_BINDING,
            .id = v.id,
        };
        DYNAMIC_ARRAY_PUSH(&w.analysis->constant_items, item);
    }

    w.binding_set = v.id;
    analyze_prepare_pattern(w, AST_VIEW_CHILD_PATTERN(v, .pattern));
    analyze_prepare_expression(w, AST_VIEW_CHILD_EXPRESSION(v, .value), v.data->kind == AST_BINDING_SET_CONSTANT);
}
