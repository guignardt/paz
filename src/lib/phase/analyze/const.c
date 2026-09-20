#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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
    AstNodes ast,
    OUT(AnConstantItem) dst
);

static int sort_constant_items_visit(
    AnConstantItem item,
    AstNodes nodes,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps(
    AnConstantItem item,
    AstNodes nodes,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_expression(
    AstExpressionView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_binding_set(
    AstBindingSetView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static int sort_constant_items_visit_deps_pattern(
    AstPatternView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
);

static uint32_t constant_item_key(AnConstantItem item);

static void evaluate_constant_item(AnConstantItem item, AstNodes nodes, Analysis* a);

static AnConstant evaluate_expression(AstExpressionView, Analysis* a);

static AnTypeId pattern_explicit_type(AstPatternView p, Analysis* a);

static void match_constant_to_pattern(AnConstant c, AstPatternView p, Analysis* a);

void analyze_const(Analysis* a, AstNodes ast) {
    BOXED(AnConstantItem) sorted_items =
        alloc_or_exit(a->constant_items.len * sizeof(AnConstantItem));
    if (sort_constant_items(a, ast, sorted_items)) {
        goto analyze_const_end;
    }

    for (size_t i = 0; i < a->constant_items.len; i++) {
        AnConstantItem item = sorted_items[i];
        evaluate_constant_item(item, ast, a);
    }

analyze_const_end:
    free(sorted_items);
    return;
}

// returns nonzero iff a cycle was found
static int sort_constant_items(
    Analysis* a,
    AstNodes nodes,
    OUT(AnConstantItem) dst
) {
    AnConstantItems items = a->constant_items;
    // keys are `AstExpression*` `AstBinding*`
    IdHashTable visited = id_hash_table_new(sizeof(uint8_t));
    for (size_t i = 0; i < items.len; i++) {
        AnConstantItem item = items.data[i];
        uint8_t unvisited = MARK_UNVISITED;
        id_hash_table_insert(&visited, constant_item_key(item), &unvisited);
    }

    int r = 0;
    OUT(AnConstantItem) next_item = dst + items.len;
    for (size_t i = 0; i < items.len; i++) {
        AnConstantItem item = items.data[i];
        if (
            *(uint8_t const*)id_hash_table_get(visited, constant_item_key(item))
            != MARK_UNVISITED
        ) {
            continue;
        }
        r |= sort_constant_items_visit(item, nodes, &visited, a, &next_item);
    }

    id_hash_table_free(&visited);
    return r;
}

static int sort_constant_items_visit(
    AnConstantItem item,
    AstNodes nodes,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    uint8_t* mark = id_hash_table_get_mut(visited, constant_item_key(item));

    if (*mark == MARK_PERMANENT) return 0;
    if (*mark == MARK_TEMPORARY) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("found cycle in constant bindings and expressions");
        report_end();
        return -1;
    }

    *mark = MARK_TEMPORARY;

    int r = sort_constant_items_visit_deps(item, nodes, visited, a, dst);

    *mark = MARK_PERMANENT;

    OUT(AnConstantItem) item_dst = --(*dst);
    *item_dst = item;

    return r;
}

static int sort_constant_items_visit_deps(
    AnConstantItem item,
    AstNodes nodes,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    switch (item.kind) {
        case AN_CONSTANT_ITEM_EXPRESSION:
            return sort_constant_items_visit_deps_expression(
                AST_VIEW_EXPRESSION(nodes, item.id),
                visited,
                a,
                dst
            );
        case AN_CONSTANT_ITEM_BINDING:
            return sort_constant_items_visit_deps_binding_set(
                AST_VIEW_BINDING_SET(nodes, item.id),
                visited,
                a,
                dst
            );
    }
}

static int sort_constant_items_visit_deps_expression(
    AstExpressionView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    String name;
    AnBindingLocation* binding_location;
    AnBindingId binding_id;
    AnBinding binding;
    AnConstantItem item;
    AstExpressionView tail;
    AstExpressionView output_type;
    int r = 0;
    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK:
            for (
                AstBindingSetView p = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
                p.data;
                p = AST_VIEW_CHILD_BINDING_SET(p, .next)
            ) {
                r |= sort_constant_items_visit_deps_binding_set(p, visited, a, dst);
            }
            tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
            if (tail.data) {
                r |= sort_constant_items_visit_deps_expression(tail, visited, a, dst);
            }
            break;

        case AST_EXPRESSION_REFERENCE:
            binding_location = (AnBindingLocation*)
                id_hash_table_get(a->reference_binding_locations, v.id);
            name = v.data->as.reference.identifier.string;
            if (an_find_binding(a->scope_bindings, binding_location->location, name, &binding_id)) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("`const` binding `%.*s` not found", (int)name.len, name.data);
                report_end();
                return -1;
            }

            binding_location->binding = binding_id;

            binding = *an_get_binding(a->scope_bindings, binding_id);
            // the following condition can be false only if the binding is built-in
            // in that case, we don't need to process it as a constant item since its value
            // is already known
            if (binding.source_binding_set != ID_SENTINEL) {
                item = (AnConstantItem) { .kind = AN_CONSTANT_ITEM_BINDING, .id = binding.source_binding_set };
                r |= sort_constant_items_visit(item, v.nodes, visited, a, dst);
            }

            break;

        case AST_EXPRESSION_FUNCTION:
            r |= sort_constant_items_visit_deps_pattern(AST_VIEW_CHILD_PATTERN(v, .as.function.input), visited, a, dst);
            output_type = AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type);
            if (output_type.data) {
                r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type), visited, a, dst);
            }
            break;

        case AST_EXPRESSION_UNARY_OPERATION:
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.unary_operation.operand), visited, a, dst);
            break;

        case AST_EXPRESSION_BINARY_OPERATION:
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.lhs), visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.rhs), visited, a, dst);
            break;

        case AST_EXPRESSION_CONDITIONAL:
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.condition), visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_true), visited, a, dst);
            r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_false), visited, a, dst);
            break;

        case AST_EXPRESSION_LITERAL_INT:
            break;
    }

    return r;
}

static int sort_constant_items_visit_deps_binding_set(
    AstBindingSetView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    int r = 0;
    r |= sort_constant_items_visit_deps_pattern(AST_VIEW_CHILD_PATTERN(v, .pattern), visited, a, dst);
    r |= sort_constant_items_visit_deps_expression(AST_VIEW_CHILD_EXPRESSION(v, .value), visited, a, dst);
    return r;
}

static int sort_constant_items_visit_deps_pattern(
    AstPatternView v,
    IdHashTable* visited,
    Analysis* a,
    OUT(AnConstantItem)* dst
) {
    int r = 0;
    AstExpressionView type_bound;
    switch (v.data->kind) {
        case AST_PATTERN_VARIABLE:
            type_bound = AST_VIEW_CHILD_EXPRESSION(v, .as.variable.type_bound);
            if (type_bound.data) {
                r |= sort_constant_items_visit_deps_expression(type_bound, visited, a, dst);
            }
            break;
    }
    return r;
}

static uint32_t constant_item_key(AnConstantItem item) {
    return (item.id << 1) | (item.kind == AN_CONSTANT_ITEM_BINDING);
}

static void evaluate_constant_item(AnConstantItem item, AstNodes nodes, Analysis* a) {
    AnConstant value;
    AstBindingSetView binding_set;
    switch (item.kind) {
        case AN_CONSTANT_ITEM_EXPRESSION:
            value = evaluate_expression(AST_VIEW_EXPRESSION(nodes, item.id), a);
            id_hash_table_insert(&a->constants, item.id, &value);
            break;
        
        case AN_CONSTANT_ITEM_BINDING:
            binding_set = AST_VIEW_BINDING_SET(nodes, item.id);
            value = *(AnConstant const*)id_hash_table_get(a->constants, binding_set.data->value);
            match_constant_to_pattern(value, AST_VIEW_CHILD_PATTERN(binding_set, .pattern), a);
            break;
    }
}

static AnConstant evaluate_expression(AstExpressionView v, Analysis* a) {
    AnBinding binding;
    AnBindingLocation binding_location;
    AnTypeId input_type;
    AnConstant output_type_value;
    AnTypeId output_type;
    AnFunctionId function_id;
    AnType type;
    AnTypeId type_id;
    switch (v.data->kind) {
        case AST_EXPRESSION_REFERENCE:
            binding_location = *(AnBindingLocation const*)
                id_hash_table_get(a->reference_binding_locations, v.id);
            binding = *an_get_binding(a->scope_bindings, binding_location.binding);
            assert(binding.kind == AN_BINDING_CONSTANT);
            return binding.as.constant;

        case AST_EXPRESSION_FUNCTION:
            input_type = pattern_explicit_type(AST_VIEW_CHILD_PATTERN(v, .as.function.input), a);
            if (v.data->as.function.output_type == ID_SENTINEL) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("missing output type bound in function");
                report_end();
                output_type = AN_TYPE_INVALID;
            } else {
                output_type_value = evaluate_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type), a);
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

            function_id = *(AnFunctionId const*)id_hash_table_get(a->function_ids, v.id);
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

static AnTypeId pattern_explicit_type(AstPatternView p, Analysis* a) {
    AstExpressionView type_bound;
    AnConstant type_value;
    switch (p.data->kind) {
        case AST_PATTERN_VARIABLE:
            type_bound = AST_VIEW_CHILD_EXPRESSION(p, .as.variable.type_bound);
            if (!type_bound.data) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("missing type bound in fully bound pattern");
                report_end();
                return AN_TYPE_ID_INVALID;
            }
            type_value = evaluate_expression(type_bound, a);
            if (type_value.kind != AN_TYPE_ID_TYPE) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("type bound didn't evaluate to type");
                report_end();
                return AN_TYPE_ID_INVALID;
            }
            return type_value.as.type;
    }
}

static void match_constant_to_pattern(AnConstant c, AstPatternView p, Analysis* a) {
    AnConstant type_bound_value;
    AnBindingId binding_id;
    AnBinding* binding;
    switch (p.data->kind) {
        case AST_PATTERN_VARIABLE:
            binding_id = *(AnBindingId const*)id_hash_table_get(a->variable_bindings, p.id);
            binding = an_get_binding_mut(&a->scope_bindings, binding_id);
            assert(binding->kind == AN_BINDING_CONSTANT);
            binding->as.constant = (AnConstant) {
                .kind = AN_CONSTANT_INVALID,
                .type = AN_TYPE_INVALID
            };

            if (p.data->as.variable.type_bound != ID_SENTINEL) {
                type_bound_value = *(AnConstant const*)
                    id_hash_table_get(a->constants, p.data->as.variable.type_bound);
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
