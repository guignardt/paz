#include "phase/analyze.h"
#include "diagnostic/report.h"

typedef enum EvResult {
    EV_RESULT_SUCCESS,
    EV_RESULT_ERROR,
    EV_RESULT_CYCLE,
} EvResult;

typedef enum EvStatus {
    EV_STATUS_NONE = 0,
    EV_STATUS_STARTED = 1,
    EV_STATUS_SUCCESS = 2,
    EV_STATUS_ERROR = 3,
} EvStatus;

typedef struct Evaluator {
    Analysis* analysis;
    BOXED(uint8_t) expression_types;
    IdHashTable /* expression id -> status */ expressions;
} Evaluator;

#define BUBBLE_CYCLE(v) do { if ((v) == EV_RESULT_CYCLE) return EV_RESULT_CYCLE; } while (0)

// #define BUBBLE_ERROR(v) do {                                \
//     EvResult result = (v);                                  \
//     if (result /* is cycle or error */) return result;      \
// } while (0)

// #define BAIL_IF(v) do { if(v) return -1; } while(0);

static EvResult ev_expression_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_block_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_reference_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_function_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_unary_operation_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_binary_operation_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_conditional_type(AstExpressionView v, Evaluator* ev);
static EvResult ev_literal_int_type(AstExpressionView v, Evaluator* ev);

static EvResult ev_pattern_type(AstPatternView v, Evaluator* ev);

static EvResult ev_binding_set_type(AstBindingSetView v, Evaluator* ev);

static EvResult ev_expression_type(AstExpressionView v, Evaluator* ev) {
    switch (ev->expression_types[v.id]) {
        case EV_STATUS_NONE: ev->expression_types[v.id] = EV_STATUS_STARTED;
        case EV_STATUS_STARTED: return -1;
        case EV_STATUS_SUCCESS:
        case EV_STATUS_ERROR:
            return 0;
    }

    int r;
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
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        BUBBLE_CYCLE(ev_binding_set_type(b, ev));
    }

    AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
    if (!tail.data) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("missing tail in block");
        report_end();
        return EV_RESULT_ERROR;
    }
    
    BUBBLE_CYCLE(ev_expression_type(tail, ev));
    ev->analysis->expression_types[v.id] = ev->analysis->expression_types[tail.id];
    
    return EV_RESULT_SUCCESS;
}

static EvResult ev_reference_type(AstExpressionView v, Evaluator* ev) {
    AnBindingId bid = *(AnBindingId const*)
        id_hash_table_get(ev->analysis->reference_binding_locations, v.id);
    AnBinding b = *an_get_binding(ev->analysis->scope_bindings, bid);

    AstPatternView pattern = AST_VIEW_PATTERN(v.nodes, b.pattern);
    if (!pattern.data) {
        // we have a built-in, type is already computed
        return EV_RESULT_SUCCESS;
    }

    EvResult r = ev_pattern_type(pattern, ev);
    ev->analysis->expression_types[v.id] = an_get_binding(ev->analysis->scope_bindings, bid)->type;
    return r;
}

static EvResult ev_function_type(AstExpressionView v, Evaluator* ev) {
    BUBBLE_CYCLE(ev_pattern_type(AST_VIEW_CHILD_PATTERN(v, .as.function.input), ev));
    bool has_output_type_bound = v.data->as.function.output_type != ID_SENTINEL;
    if (has_output_type_bound) {
        BUBBLE_CYCLE(ev_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output_type), ev));
    }
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.function.output), ev));

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
            if (value.type != output_type) {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("type mismatch");
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

    return 0;
}

static EvResult ev_unary_operation_type(AstExpressionView v, Evaluator* ev) {
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.unary_operation.operand), ev));
    
    AnTypeId type;
    switch (v.data->as.unary_operation.operator) {
        case AST_OPERATOR_BREAK:
        case AST_OPERATOR_CONTINUE:
            type = AN_TYPE_ID_NEVER;
            break;
    }
    ev->analysis->expression_types[v.id] = type;

    return 0;
}

static EvResult ev_binary_operation_type(AstExpressionView v, Evaluator* ev) {
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.lhs), ev));
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.rhs), ev));

    AnTypeId type_id = AN_TYPE_ID_INVALID;
    AnTypeId callee_type_id;
    AnType callee_type;
    switch (v.data->as.binary_operation.operator) {
        case AST_OPERATOR_CALL:
            callee_type_id = ev->analysis->expression_types[v.data->as.binary_operation.lhs];
            callee_type = an_types_get(ev->analysis->types, callee_type_id);
            if (callee_type.kind == AN_TYPE_FUNCTION) {
                type_id = callee_type.as.function.output;
            } else {
                report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
                report_message("tried to call non-function");
                report_end();
            }
            break;
        
        default:
            type_id = AN_TYPE_ID_INT;
            break;
    }
    ev->analysis->expression_types[v.id] = type_id;

    return 0;
}

static EvResult ev_conditional_type(AstExpressionView v, Evaluator* ev) {
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.condition), ev));
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_true), ev));
    BUBBLE_CYCLE(ev_expression_type(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_false), ev));

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

    return 0;
}

static EvResult ev_literal_int_type(AstExpressionView v, Evaluator* ev) {
    ev->analysis->expression_types[v.id] = AN_TYPE_ID_INT;
}
