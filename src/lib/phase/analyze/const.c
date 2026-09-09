#include <assert.h>

#include "phase/analyze.h"

#include "diagnostic/report.h"

typedef enum Mark {
    MARK_UNVISITED,
    MARK_TEMPORARY,
    MARK_PERMANENT,
} Mark;

// finds a topological ordering of the constant items
// returns nonzero if a cycle was found or a binding wasn't resolved
static int sort_constant_items(
    Analysis* a,
    OUT(AnConstantItem) dst
);

static int sort_constant_items_visit(
    AnConstantItem item,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps(
    AnConstantItem item,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_expression(
    AstExpression const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_binding(
    AstBinding const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_pattern(
    AstPattern const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static intptr_t constant_item_key(AnConstantItem item);

static void evaluate_constant_item(AnConstantItem item, Analysis* a);

static AnConstant evaluate_expression(AstExpression const* v, Analysis* a);

static AnTypeId pattern_explicit_type(AstPattern const* p, Analysis* a);

static void match_constant_to_pattern(AnConstant c, AstPattern const* p, Analysis* a);

void analyze_const(Analyzer w) {
    OWNED(AnConstantItem) sorted_items =
        alloc_or_exit(w.analysis->constant_items.len * sizeof(AnConstantItem));
    if (sort_constant_items(w.analysis, sorted_items)) {
        return;
    }

    for (size_t i = 0; i < w.analysis->constant_items.len; i++) {
        AnConstantItem item = sorted_items[i];
        evaluate_constant_item(item, w.analysis);
    }
}

// returns nonzero iff a cycle was found
static int sort_constant_items(
    Analysis* a,
    OUT(AnConstantItem) dst
) {
    AnConstantItems items = a->constant_items;
    // keys are `AstExpression*` `AstBinding*`
    PtrHashTable visited = ptr_hash_table_new(sizeof(uint8_t));
    for (size_t i = 0; i < items.len; i++) {
        AnConstantItem item = items.data[i];
        uint8_t unvisited = MARK_UNVISITED;
        ptr_hash_table_insert(&visited, constant_item_key(item), &unvisited);
    }

    int r = 0;
    OUT(AnConstantItem) next_item = dst + items.len;
    for (size_t i = 0; i < items.len; i++) {
        AnConstantItem item = items.data[i];
        if (
            *(uint8_t const*)ptr_hash_table_get(visited, constant_item_key(item))
            != MARK_UNVISITED
        ) {
            continue;
        }
        r |= sort_constant_items_visit(item, &visited, a, &next_item);
    }

    ptr_hash_table_free(&visited);
    return r;
}

static int sort_constant_items_visit(
    AnConstantItem item,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    uint8_t* mark = ptr_hash_table_get_mut(visited, constant_item_key(item));

    if (*mark == MARK_PERMANENT) return 0;
    if (*mark == MARK_TEMPORARY) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("found cycle in constant bindings and expressions");
        report_end();
        return -1;
    }

    *mark = MARK_TEMPORARY;

    int r = sort_constant_items_visit_deps(item, visited, a, dst);

    *mark = MARK_PERMANENT;

    OUT(AnConstantItem) item_dst = --(*dst);
    *item_dst = item;

    return r;
}

static int sort_constant_items_visit_deps(
    AnConstantItem item,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    switch (item.kind) {
        case AN_CONSTANT_ITEM_EXPRESSION:
            return sort_constant_items_visit_deps_expression(item.as.expression, visited, a, dst);
        case AN_CONSTANT_ITEM_BINDING:
            return sort_constant_items_visit_deps_binding(item.as.binding, visited, a, dst);
    }
}

static int sort_constant_items_visit_deps_expression(
    AstExpression const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    String name;
    AnScopeId scope;
    AnBindingId binding_id;
    AnBinding binding;
    AnConstantItem item;
    int r = 0;
    switch (v->kind) {
        case AST_EXPRESSION_BLOCK:
            for (AstBinding const* p = v->as.block.bindings; p; p = p->next) {
                r |= sort_constant_items_visit_deps_binding(p, visited, a, dst);
            }
            if (v->as.block.tail) {
                r |= sort_constant_items_visit_deps_expression(v->as.block.tail, visited, a, dst);
            }
            break;

        case AST_EXPRESSION_REFERENCE:
            scope = *(AnScopeId const*)ptr_hash_table_get(a->reference_scopes.table, (intptr_t)v);
            name = v->as.reference.identifier.string;
            if (an_find_binding(a->scope_bindings, scope, name, &binding_id)) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("`const` binding `%.*s` not found", (int)name.len, name.data);
                report_end();
                return -1;
            }

            ptr_hash_table_insert(&a->binding_ids.table, (intptr_t)v, &binding_id);

            binding = *an_get_binding(a->scope_bindings, binding_id);
            // the following condition can be false only if the binding is built-in
            // in that case, we don't need to process it as a constant item since its value
            // is already known
            if (binding.source_binding) {
                item = (AnConstantItem) { .kind = AN_CONSTANT_ITEM_BINDING, .as.binding = binding.source_binding };
                r |= sort_constant_items_visit(item, visited, a, dst);
            }

            break;

        case AST_EXPRESSION_FUNCTION:
            r |= sort_constant_items_visit_deps_pattern(v->as.function.input, visited, a, dst);
            if (v->as.function.output_type) {
                r |= sort_constant_items_visit_deps_expression(v->as.function.output_type, visited, a, dst);
            }
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            r |= sort_constant_items_visit_deps_expression(v->as.unary_operation.operand, visited, a, dst);
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            r |= sort_constant_items_visit_deps_expression(v->as.binary_operation.lhs, visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(v->as.binary_operation.rhs, visited, a, dst);
            break;

        case AST_EXPRESSION_CONDITIONAL:
            r |= sort_constant_items_visit_deps_expression(v->as.conditional.condition, visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(v->as.conditional.if_true, visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(v->as.conditional.if_false, visited, a, dst);
            break;

        case AST_EXPRESSION_LITERAL_INT:
            break;
    }

    return r;
}

static int sort_constant_items_visit_deps_binding(
    AstBinding const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    int r = 0;
    r |= sort_constant_items_visit_deps_pattern(v->pattern, visited, a, dst);
    r |= sort_constant_items_visit_deps_expression(v->value, visited, a, dst);
    return r;
}

static int sort_constant_items_visit_deps_pattern(
    AstPattern const* v,
    PtrHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    int r = 0;
    switch (v->kind) {
        case AST_PATTERN_VARIABLE:
            if (v->as.variable.type_bound) {
                r |= sort_constant_items_visit_deps_expression(v->as.variable.type_bound, visited, a, dst);
            }
            break;
    }
    return r;
}

static intptr_t constant_item_key(AnConstantItem item) {
    switch (item.kind) {
        case AN_CONSTANT_ITEM_EXPRESSION: return (intptr_t)item.as.expression;
        case AN_CONSTANT_ITEM_BINDING: return (intptr_t)item.as.binding;
    }
}

static void evaluate_constant_item(AnConstantItem item, Analysis* a) {
    AnConstant value;
    switch (item.kind) {
        case AN_CONSTANT_ITEM_EXPRESSION:
            value = evaluate_expression(item.as.expression, a);
            ptr_hash_table_insert(&a->constants.table, (intptr_t)item.as.expression, &value);
            break;
        
        case AN_CONSTANT_ITEM_BINDING:
            value = evaluate_expression(item.as.binding->value, a);
            match_constant_to_pattern(value, item.as.binding->pattern, a);
            break;
    }
}

static AnConstant evaluate_expression(AstExpression const* v, Analysis* a) {
    AnBindingId binding_id;
    AnBinding binding;
    AnTypeId input_type;
    AnConstant output_type_value;
    AnTypeId output_type;
    AnFunctionId function_id;
    AnType type;
    AnTypeId type_id;
    switch (v->kind) {
        case AST_EXPRESSION_REFERENCE:
            binding_id = *(AnBindingId const*)ptr_hash_table_get(a->binding_ids.table, (intptr_t)v);
            binding = *an_get_binding(a->scope_bindings, binding_id);
            assert(binding.kind == AN_BINDING_CONSTANT);
            return binding.as.constant;

        case AST_EXPRESSION_FUNCTION:
            input_type = pattern_explicit_type(v->as.function.input, a);
            if (!v->as.function.output_type) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("missing output type bound in function");
                report_end();
                output_type = AN_TYPE_INVALID;
            } else {
                output_type_value = evaluate_expression(v->as.function.output_type, a);
                if (output_type_value.kind != AN_CONSTANT_TYPE) {
                    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                    report_message("type bound didn't evaluate to type");
                    report_end();
                    output_type = AN_TYPE_INVALID;
                } else {
                    output_type = output_type_value.as.type;
                }
            }

            type = (AnType) {
                .kind = AN_TYPE_FUNCTION,
                .as.function = (AnFunctionType) { .input = input_type, .output = output_type },
                .size = 1, // TODO: function capture data
            };
            type_id = an_types_get_id_or_insert(&a->types, type);

            function_id = *(AnFunctionId const*)ptr_hash_table_get(a->function_ids.table, (intptr_t)v);
            return (AnConstant) {
                .kind = AN_CONSTANT_FUNCTION,
                .as.function = function_id,
                .type = type_id,
            };

        default:
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("invalid constant expression");
            report_end();
            return (AnConstant) { .kind = AN_CONSTANT_INVALID, .type = AN_TYPE_INVALID };
    }
}

static AnTypeId pattern_explicit_type(AstPattern const* p, Analysis* a) {
    AnConstant type_value;
    switch (p->kind) {
        case AST_PATTERN_VARIABLE:
            if (!p->as.variable.type_bound) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("missing type bound in fully bound pattern");
                report_end();
                return AN_TYPE_ID_INVALID;
            }
            type_value = evaluate_expression(p->as.variable.type_bound, a);
            if (type_value.kind != AN_TYPE_ID_TYPE) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("type bound didn't evaluate to type");
                report_end();
                return AN_TYPE_ID_INVALID;
            }
            return type_value.as.type;
    }
}

static void match_constant_to_pattern(AnConstant c, AstPattern const* p, Analysis* a) {
    AnConstant type_bound_value;
    AnBindingId binding_id;
    AnBinding* binding;
    switch (p->kind) {
        case AST_PATTERN_VARIABLE:
            binding_id = *(AnBindingId const*)ptr_hash_table_get(a->binding_ids.table, (intptr_t)p);
            binding = an_get_binding_mut(&a->scope_bindings, binding_id);
            assert(binding->kind == AN_BINDING_CONSTANT);
            binding->as.constant = (AnConstant) {
                .kind = AN_CONSTANT_INVALID,
                .type = AN_TYPE_INVALID
            };

            if (p->as.variable.type_bound) {
                type_bound_value = *(AnConstant const*)
                    ptr_hash_table_get(a->constants.table, (intptr_t)p->as.variable.type_bound);
                if (type_bound_value.kind != AN_CONSTANT_TYPE) {
                    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                    report_message("invalid type bound: bound value didn't evaluate to a type");
                    report_end();
                    return;
                }

                binding->as.constant.type = type_bound_value.as.type;
                if (c.type != type_bound_value.as.type) {
                    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                    report_message("mismatched types");
                    report_end();
                    break;
                }
            }

            binding->as.constant = c;

            break;
    }
}
