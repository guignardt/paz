#include <assert.h>
#include <string.h>

#include "phase/analyze.h"
#include "diagnostic/report.h"

typedef enum EvResult {
    EV_RESULT_SUCCESS = 0,
    EV_RESULT_ERROR = 1,
    EV_RESULT_CYCLE = 2,
} EvResult;

typedef enum EvStatus {
    EV_STATUS_NONE = 0,
    EV_STATUS_STARTED = 1,
    EV_STATUS_SUCCESS = 2,
    EV_STATUS_ERROR = 3,
} EvStatus;

typedef struct Evaluator {
    Analysis* analysis;
    BOXED(uint8_t /* status */) expression_types;
    IdHashTable /* expression id -> status as uint8_t */ expression_values;
    BOXED(uint8_t /* status */) pattern_types;
    IdHashTable /* pattern id -> status as uint8_t */ pattern_values;
} Evaluator;

#define SUB_EV(v, ...) do {                                 \
    EvResult SUB_EV_result = (v);                           \
    switch (SUB_EV_result) {                                \
        case EV_RESULT_SUCCESS: break;                      \
        case EV_RESULT_ERROR:                               \
            __VA_OPT__(__VA_ARGS__ = EV_RESULT_ERROR;)      \
            break;                                          \
        case EV_RESULT_CYCLE: return EV_RESULT_CYCLE;       \
    }                                                       \
} while(0);

static EvResult ev_expression_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_block_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_reference_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_function_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_unary_operation_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_binary_operation_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_conditional_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_literal_int_type(AstExpressionView v, Evaluator* ev);

static EvResult ev_expression_value(AstExpressionView v, Evaluator* ev);
static EvResult ev_reference_value(AstExpressionView v, Evaluator* ev);
static EvResult ev_function_value(AstExpressionView v, Evaluator* ev);

static EvResult ev_pattern_type(AstPatternView v, Evaluator* ev);

static EvResult ev_pattern_value(AstPatternView v, Evaluator* ev);

static EvResult ev_binding_set_type(AstBindingSetView v, Evaluator* ev);

static EvResult ev_binding_set_value(AstBindingSetView v, Evaluator* ev);

#define STOP_ON_CYCLE(v) do {                           \
    if (v == EV_RESULT_CYCLE) {                         \
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE); \
        report_message("found evaluation cycle");       \
        report_end();                                   \
        return;                                         \
    }                                                   \
} while (0)

void analyze_evaluate(Analysis* a) {
    AstNodes nodes = ast_node_view_nodes(a->root);

    BOXED(uint8_t) expression_types = alloc_or_exit(nodes.expressions.len * sizeof(uint8_t));
    memset(expression_types, EV_STATUS_NONE, nodes.expressions.len);
    BOXED(uint8_t) pattern_types = alloc_or_exit(nodes.patterns.len * sizeof(uint8_t));
    memset(pattern_types, EV_STATUS_NONE, nodes.patterns.len);

    Evaluator ev = {
        .analysis = a,
        .expression_types = expression_types,
        .expression_values = id_hash_table_new(sizeof(uint8_t)),
        .pattern_types = pattern_types,
        .pattern_values = id_hash_table_new(sizeof(uint8_t)),
    };

    for (AstBindingSetId id = 0; id < nodes.binding_sets.len; id++) {
        STOP_ON_CYCLE(ev_binding_set_type(AST_VIEW_BINDING_SET(nodes, id), &ev));
    }
    for (AstPatternId id = 0; id < nodes.patterns.len; id++) {
        STOP_ON_CYCLE(ev_pattern_type(AST_VIEW_PATTERN(nodes, id), &ev));
    }
    for (AstExpressionId id = 0; id < nodes.expressions.len; id++) {
        STOP_ON_CYCLE(ev_expression_type(AST_VIEW_EXPRESSION(nodes, id), &ev));
    }
}

static EvResult ev_expression_type(AstExpressionView v, Evaluator* ev) {
    switch (ev->expression_types[v.id]) {
        case EV_STATUS_NONE: ev->expression_types[v.id] = EV_STATUS_STARTED; break;
        case EV_STATUS_STARTED: return EV_RESULT_CYCLE;
        case EV_STATUS_SUCCESS: return EV_RESULT_SUCCESS;
        case EV_STATUS_ERROR: return EV_RESULT_ERROR;
    }

    EvResult r;
    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK: r = ev_block_type(v, ev); break;
        case AST_EXPRESSION_REFERENCE: r = ev_reference_type(v, ev); break;
        case AST_EXPRESSION_FUNCTION: r = ev_function_type(v, ev); break;
        case AST_EXPRESSION_UNARY_OPERATION: r = ev_unary_operation_type(v, ev); break;
        case AST_EXPRESSION_BINARY_OPERATION: r = ev_binary_operation_type(v, ev); break;
        case AST_EXPRESSION_CONDITIONAL: r = ev_conditional_type(v, ev); break;
        case AST_EXPRESSION_LITERAL_INT: r = ev_literal_int_type(v, ev); break;
    }

    switch (r) {
        case EV_RESULT_SUCCESS: ev->expression_types[v.id] = EV_STATUS_SUCCESS;
        case EV_RESULT_ERROR: ev->expression_types[v.id] = EV_STATUS_ERROR;
        default: break;
    }

    return r;
}

static EvResult ev_block_type(AstExpressionView v, Evaluator* ev) {
    EvResult r = EV_RESULT_SUCCESS;

    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        SUB_EV(ev_binding_set_type(b, ev), r);
    }

    AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
    if (!tail.data) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("missing tail in block");
        report_end();
        return EV_RESULT_ERROR;
    }
    
    SUB_EV(ev_expression_type(tail, ev), r);
    ev->analysis->expression_types[v.id] = ev->analysis->expression_types[tail.id];
    
    return r;
}

static EvResult ev_reference_type(AstExpressionView v, Evaluator* ev) {
    AnBindingLocation const* binding_location = (AnBindingLocation const*)
        id_hash_table_get(ev->analysis->reference_binding_locations, v.id);
    AnBindingId bid = binding_location->binding;
    AnBinding b = *an_get_binding(ev->analysis->scope_bindings, bid);

    EvResult r = EV_RESULT_SUCCESS;
    AstPatternView pattern = AST_VIEW_PATTERN(v.nodes, b.top_level_pattern);
    if (pattern.data) {
        SUB_EV(ev_pattern_type(pattern, ev), r);
    }
    // otherwise we have a built-in, type is already computed

    AnTypeId type = an_get_binding(ev->analysis->scope_bindings, bid)->type;
    ev->analysis->expression_types[v.id] = type;
    return r;
}

static EvResult ev_function_type(AstExpressionView v, Evaluator* ev) {
    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_pattern_type(AST_VIEW_CHILD_PATTERN(v, .as.function.input), ev), r);
    bool has_output_type_bound = v.data->as.function.output_type != ID_SENTINEL;
    if (has_output_type_bound) {
        SUB_EV(ev_expression_value(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type), ev), r);
    }
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output), ev), r);

    AnTypeId input_type = ev->analysis->pattern_types[v.data->as.function.input];
    AnTypeId output_type = ev->analysis->expression_types[v.data->as.function.output];
    if (has_output_type_bound) {
        AnConstant value = *(AnConstant const*)
            id_hash_table_get(ev->analysis->constants, v.data->as.function.output_type);
        if (value.kind != AN_CONSTANT_TYPE) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("type bound didn't evaluate to type");
            report_end();
        } else {
            if (value.as.type != output_type) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("type mismatch in function output");
                report_end();
            }
            output_type = value.type;
        }
    }

    AnType type = {
        .kind = AN_TYPE_FUNCTION,
        .as.function = {
            .input = input_type,
            .output = output_type,
        },
        .size = 1, // TODO: captured data
    };
    AnTypeId type_id = an_types_get_id_or_insert(&ev->analysis->types, type);
    ev->analysis->expression_types[v.id] = type_id;

    return r;
}

static EvResult ev_unary_operation_type(AstExpressionView v, Evaluator* ev) {
    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.unary_operation.operand), ev), r);

    AstUnaryOperation w = v.data->as.unary_operation;

    AnTypeId type;
    switch (w.operator) {
        case AST_OPERATOR_NEG:
            type = AN_TYPE_ID_INT;
            if (ev->analysis->expression_types[w.operand] != AN_TYPE_ID_INT) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("expected `Int` type in arithmetic operation");
                report_end();
                r = EV_RESULT_ERROR;
            }
            break;
    }
    ev->analysis->expression_types[v.id] = type;

    return r;
}

static EvResult ev_binary_operation_type(AstExpressionView v, Evaluator* ev) {
    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.lhs), ev), r);
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.rhs), ev), r);

    AstBinaryOperation w = v.data->as.binary_operation;

    AnTypeId type_id = AN_TYPE_ID_INVALID;
    AnTypeId callee_type_id;
    AnType callee_type;
    switch (w.operator) {
        case AST_OPERATOR_CALL:
            callee_type_id = ev->analysis->expression_types[w.lhs];
            callee_type = an_types_get(ev->analysis->types, callee_type_id);
            if (callee_type.kind != AN_TYPE_FUNCTION) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("tried to call non-function");
                report_end();
                r = EV_RESULT_ERROR;
                break;
            }
            type_id = callee_type.as.function.output;
            if (callee_type.as.function.input != ev->analysis->expression_types[w.rhs]) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("type mismatch in function call");
                report_end();
                r = EV_RESULT_ERROR;
                break;
            }
            break;
        
        // FIXME: enumerate arithmetic operator cases>
        default:
            type_id = AN_TYPE_ID_INT;
            if (
                ev->analysis->expression_types[w.lhs] != AN_TYPE_ID_INT
                || ev->analysis->expression_types[w.rhs] != AN_TYPE_ID_INT
            ) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("expected `Int` type in arithmetic operation");
                report_end();
                r = EV_RESULT_ERROR;
            }
            break;
    }
    ev->analysis->expression_types[v.id] = type_id;

    return r;
}

static EvResult ev_conditional_type(AstExpressionView v, Evaluator* ev) {
    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.condition), ev), r);
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_true), ev), r);
    SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_false), ev), r);

    if (ev->analysis->expression_types[v.data->as.conditional.condition] != AN_TYPE_ID_INT) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("condition wasn't of type `Int`");
        report_end();
    }

    AnTypeId type;
    if (
        ev->analysis->expression_types[v.data->as.conditional.if_true]
        != ev->analysis->expression_types[v.data->as.conditional.if_false]
    ) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("type mismatch in condition");
        report_end();
        type = AN_TYPE_ID_INVALID;
    } else {
        type = ev->analysis->expression_types[v.data->as.conditional.if_true];
    }

    ev->analysis->expression_types[v.id] = type;

    return r;
}

static EvResult ev_literal_int_type(AstExpressionView v, Evaluator* ev) {
    ev->analysis->expression_types[v.id] = AN_TYPE_ID_INT;
    return EV_RESULT_SUCCESS;
}

static EvResult ev_expression_value(AstExpressionView v, Evaluator* ev) {
    uint8_t const* ps = id_hash_table_get(ev->expression_values, v.id);
    EvStatus s = (ps) ? *ps : EV_STATUS_NONE;
    switch (s) {
        case EV_STATUS_NONE:
            s = EV_STATUS_STARTED;
            id_hash_table_insert(&ev->expression_values, v.id, &s);
            break;
        case EV_STATUS_STARTED: return EV_RESULT_CYCLE;
        case EV_STATUS_SUCCESS: return EV_RESULT_SUCCESS;
        case EV_STATUS_ERROR: return EV_RESULT_ERROR;
    }

    AnConstant invalid = {
        .kind = AN_CONSTANT_INVALID,
        .type = AN_TYPE_INVALID,
    };

    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_expression_type(v, ev), r);
    if (r == EV_RESULT_ERROR) {
        id_hash_table_insert(&ev->analysis->constants, v.id, &invalid);
        return EV_RESULT_ERROR;
    }

    switch (v.data->kind) {
        case AST_EXPRESSION_REFERENCE: r = ev_reference_value(v, ev); break;
        case AST_EXPRESSION_FUNCTION: r = ev_function_value(v, ev); break;
        default:
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("invalid constant expression");
            report_end();
            r = EV_RESULT_ERROR;
            id_hash_table_insert(&ev->analysis->constants, v.id, &invalid);
            break;
    }

    switch (r) {
        case EV_RESULT_SUCCESS: s = EV_STATUS_SUCCESS;
        case EV_RESULT_ERROR: s = EV_STATUS_ERROR;
        default: break;
    }
    id_hash_table_insert(&ev->expression_values, v.id, &s);

    return r;
}

static EvResult ev_reference_value(AstExpressionView v, Evaluator* ev) {
    AnBindingLocation const* binding_location = (AnBindingLocation const*)
        id_hash_table_get(ev->analysis->reference_binding_locations, v.id);
    AnBindingId bid = binding_location->binding;
    AnBinding b = *an_get_binding(ev->analysis->scope_bindings, bid);

    EvResult r = EV_RESULT_SUCCESS;
    AstPatternView pattern = AST_VIEW_PATTERN(v.nodes, b.top_level_pattern);
    if (pattern.data) {
        SUB_EV(ev_pattern_value(pattern, ev), r);
    }
    // otherwise we have a built-in, type is already computed

    AnBinding const* pb = an_get_binding(ev->analysis->scope_bindings, bid);
    assert(pb->kind == AN_BINDING_CONSTANT && "constant binding kind wasn't constant");
    AnConstant value = pb->as.constant;
    id_hash_table_insert(&ev->analysis->constants, v.id, &value);

    return r;
}

static EvResult ev_function_value(AstExpressionView v, Evaluator* ev) {
    AnFunctionId id = *(AnFunctionId const*)id_hash_table_get(ev->analysis->function_ids, v.id);
    AnTypeId type = ev->analysis->expression_types[v.id];
    AnConstant value = {
        .kind = AN_CONSTANT_FUNCTION,
        .as.function = id,
        .type = type,
    };
    id_hash_table_insert(&ev->analysis->constants, v.id, &value);
    return EV_RESULT_SUCCESS;
}

static EvResult ev_pattern_type(AstPatternView v, Evaluator* ev) {
    switch (ev->pattern_types[v.id]) {
        case EV_STATUS_NONE: ev->pattern_types[v.id] = EV_STATUS_STARTED; break;
        case EV_STATUS_STARTED: return EV_RESULT_CYCLE;
        case EV_STATUS_SUCCESS: return EV_RESULT_SUCCESS;
        case EV_STATUS_ERROR: return EV_RESULT_ERROR;
    }

    AstBindingSetView binding_set =
        AST_VIEW_BINDING_SET(v.nodes, ev->analysis->pattern_binding_sets[v.id]);
    EvResult r = EV_RESULT_SUCCESS;
    if (binding_set.data) {
        SUB_EV(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(binding_set, .value), ev), r);
    }

    AstExpressionView type_bound;
    AnConstant type_bound_value;
    AnTypeId type_id = AN_TYPE_INVALID;
    AnBindingId binding_id;
    switch (v.data->kind) {
        case AST_PATTERN_VARIABLE:
            type_bound = AST_VIEW_CHILD_EXPRESSION(v, .as.variable.type_bound);
            if (type_bound.data) {
                SUB_EV(ev_expression_value(type_bound, ev), r);
                type_bound_value = *(AnConstant const*)
                    id_hash_table_get(ev->analysis->constants, type_bound.id);
                if (type_bound_value.kind != AN_CONSTANT_TYPE) {
                    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                    report_message("type bound didn't evaluate to type");
                    report_end();
                    r = EV_RESULT_ERROR;
                } else {
                    type_id = type_bound_value.as.type;
                    if (binding_set.data) {
                        if (ev->analysis->expression_types[binding_set.data->value] != type_id) {
                            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                            report_message("type mismatch in binding declaration");
                            report_end();
                            r = EV_RESULT_ERROR;
                        }
                    }
                }
            }
            if (binding_set.data && type_id == AN_TYPE_ID_INVALID) {
                type_id = ev->analysis->expression_types[binding_set.data->value];
            }
            if (!binding_set.data && !type_bound.data) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("missing type bound");
                report_end();
                r = EV_RESULT_ERROR;
            }

            binding_id = *(AnBindingId const*)
                id_hash_table_get(ev->analysis->variable_bindings, v.id);
            an_get_binding_mut(&ev->analysis->scope_bindings, binding_id)->type = type_id;

            break;
    }

    switch (r) {
        case EV_RESULT_SUCCESS: ev->pattern_types[v.id] = EV_STATUS_SUCCESS;
        case EV_RESULT_ERROR: ev->pattern_types[v.id] = EV_STATUS_ERROR;
        default: break;
    }

    return r;
}

static EvResult ev_pattern_value(AstPatternView v, Evaluator* ev) {
    uint8_t const* ps = id_hash_table_get(ev->pattern_values, v.id);
    EvStatus s = (ps) ? *ps : EV_STATUS_NONE;
    switch (s) {
        case EV_STATUS_NONE:
            s = EV_STATUS_STARTED;
            id_hash_table_insert(&ev->pattern_values, v.id, &s);
            break;
        case EV_STATUS_STARTED: return EV_RESULT_CYCLE;
        case EV_STATUS_SUCCESS: return EV_RESULT_SUCCESS;
        case EV_STATUS_ERROR: return EV_RESULT_ERROR;
    }

    AnConstant invalid = {
        .kind = AN_CONSTANT_INVALID,
        .type = AN_TYPE_INVALID,
    };

    EvResult r = EV_RESULT_SUCCESS;
    SUB_EV(ev_pattern_type(v, ev), r);
    if (r == EV_RESULT_ERROR) {
        id_hash_table_insert(&ev->analysis->constants, v.id, &invalid);
        return EV_RESULT_ERROR;
    }

    AstBindingSetView binding_set =
        AST_VIEW_BINDING_SET(v.nodes, ev->analysis->pattern_binding_sets[v.id]);
    if (!binding_set.data) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("constant pattern doesn't correspond to a binding declaration");
        report_end();
    }
    SUB_EV(ev_expression_value(AST_VIEW_CHILD_EXPRESSION(binding_set, .value), ev), r);
    AnConstant value = *(AnConstant const*)
        id_hash_table_get(ev->analysis->constants, binding_set.data->value);

    AnBindingId binding_id;
    AnBinding* binding;
    switch (v.data->kind) {
        case AST_PATTERN_VARIABLE:
            binding_id = *(AnBindingId const*)
                id_hash_table_get(ev->analysis->variable_bindings, v.id);
            binding = an_get_binding_mut(&ev->analysis->scope_bindings, binding_id);
            binding->kind = AN_BINDING_CONSTANT;
            binding->as.constant = value;
    }

    switch (r) {
        case EV_RESULT_SUCCESS: s = EV_STATUS_SUCCESS;
        case EV_RESULT_ERROR: s = EV_STATUS_ERROR;
        default: break;
    }
    id_hash_table_insert(&ev->pattern_values, v.id, &s);

    return r;
}

// FIXME: binding sets are kinda weird

static EvResult ev_binding_set_type(AstBindingSetView v, Evaluator* ev) {
    switch (v.data->kind) {
        case AST_BINDING_SET_CONSTANT:
            return ev_pattern_value(AST_VIEW_CHILD_PATTERN(v, .pattern), ev);
        case AST_BINDING_SET_VARIABLE:
            return ev_pattern_type(AST_VIEW_CHILD_PATTERN(v, .pattern), ev);
    }
}

[[maybe_unused]]
static EvResult ev_binding_set_value(AstBindingSetView v, Evaluator* ev) {
    assert(v.data->kind == AST_BINDING_SET_CONSTANT);
    return ev_pattern_value(AST_VIEW_CHILD_PATTERN(v, .pattern), ev);
}
