#include "phase/compile.h"
#include "phase/emit.h"

#include "diagnostic/report.h"

typedef struct Compiler {
    AstNodes ast;
    Analysis analysis;
    Emitter* emitter;
} Compiler;

static void comp_function(AnFunctionId id, Compiler a);

static void comp_expression(AstExpressionView v, Compiler a);
static void comp_expression_block(AstExpressionView v, Compiler a);
static void comp_expression_reference(AstExpressionView v, Compiler a);
static void comp_expression_function(AstExpressionView v, Compiler a);
static void comp_expression_unary_operation(AstExpressionView v, Compiler a);
static void comp_expression_binary_operation(AstExpressionView v, Compiler a);
static void comp_expression_conditional(AstExpressionView v, Compiler a);
static void comp_expression_tuple(AstExpressionView v, Compiler a);
static void comp_expression_literal_int(AstExpressionView v, Compiler a);

static void comp_constant(AnConstant v, Compiler a);

static void comp_pattern(AstPatternView v, Compiler a);
static void comp_pattern_variable(AstPatternView v, Compiler a);
static void comp_pattern_tuple(AstPatternView v, Compiler a);

static void comp_binding_set(AstBindingSetView v, Compiler a);

Bytecode compile(AstNodes ast, Analysis a) {
    Emitter e = emitter_new();
    Compiler assembler = {
        .ast = ast,
        .analysis = a,
        .emitter = &e,
    };

    for (AnFunctionId i = 0; i < a.functions.len; i++) {
        emit_symbol(&e); // => function symbol == function id
    }
    for (AnFunctionId i = 0; i < a.functions.len; i++) {
        comp_function(i, assembler);
    }

    return emit_finish(&e);
}

static void comp_function(AnFunctionId id, Compiler a) {
    AnFunction func = a.analysis.functions.data[id];
    AstExpressionView expr = AST_VIEW_EXPRESSION(a.ast, func.expression);

    emit_symbol_location(a.emitter, id);

    emit(res)(a.emitter, func.stack_space);

    comp_pattern(AST_VIEW_CHILD_PATTERN(expr, .as.function.input), a);

    comp_expression(AST_VIEW_CHILD_EXPRESSION(expr, .as.function.output), a);
    emit_ret(a.emitter);
}

static void comp_expression(AstExpressionView v, Compiler a) {
    AnConstant const* constant_value = id_hash_table_get(a.analysis.constants, v.id);
    if (constant_value) {
        comp_constant(*constant_value, a);
        return;
    }

    switch (v.data->kind) {
        case AST_EXPRESSION_BLOCK: comp_expression_block(v, a); break;
        case AST_EXPRESSION_REFERENCE: comp_expression_reference(v, a); break;
        case AST_EXPRESSION_FUNCTION: comp_expression_function(v, a); break;
        case AST_EXPRESSION_UNARY_OPERATION: comp_expression_unary_operation(v, a); break;
        case AST_EXPRESSION_BINARY_OPERATION: comp_expression_binary_operation(v, a); break;
        case AST_EXPRESSION_CONDITIONAL: comp_expression_conditional(v, a); break;
        case AST_EXPRESSION_TUPLE: comp_expression_tuple(v, a); break;
        case AST_EXPRESSION_LITERAL_INT: comp_expression_literal_int(v, a); break;
        case AST_EXPRESSION_TUPLE_TYPE:
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("internal error: tried to compile expression that should be constant but wasn't");
            report_end();
            break;
    }
}

static void comp_expression_block(AstExpressionView v, Compiler a) {
    for (
        AstBindingSetView b = AST_VIEW_CHILD_BINDING_SET(v, .as.block.binding_sets);
        b.data;
        b = AST_VIEW_CHILD_BINDING_SET(b, .next)
    ) {
        comp_binding_set(b, a);
    }
    AstExpressionView tail = AST_VIEW_CHILD_EXPRESSION(v, .as.block.tail);
    if (tail.data) {
        comp_expression(tail, a);
    }
}

static void comp_expression_reference(AstExpressionView v, Compiler a) {
    AnBindingLocation const* pbloc = (AnBindingLocation const*)
        id_hash_table_get(a.analysis.reference_binding_locations, v.id);
    AnBindingId bid = pbloc->binding;
    AnBinding b = *an_get_binding(a.analysis.scope_bindings, bid);
    switch (b.kind) {
        case AN_BINDING_CONSTANT:
            comp_constant(b.as.constant, a);
            break;
        case AN_BINDING_VARIABLE:
            for (size_t i = b.as.variable.range.start; i < b.as.variable.range.end; i++) {
                emit(var)(a.emitter, i);
            }
            break;
    }
}

static void comp_expression_function(AstExpressionView v, Compiler a) {
    AnFunctionId fid = *(AnFunctionId const*)id_hash_table_get(a.analysis.function_ids, v.id);
    emit_loc(a.emitter, fid);
}

static void comp_expression_unary_operation(AstExpressionView v, Compiler a) {
    comp_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.unary_operation.operand), a);
    switch (v.data->as.unary_operation.operator) {
        case AST_OPERATOR_NEG: emit_negi(a.emitter); break;
    }
}

static void comp_expression_binary_operation(AstExpressionView v, Compiler a) {
    AstBinaryOperator op = v.data->as.binary_operation.operator;
    AstExpressionView lhs = AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.lhs);
    AstExpressionView rhs = AST_VIEW_CHILD_EXPRESSION(v, .as.binary_operation.rhs);
    if (v.data->as.binary_operation.operator == AST_OPERATOR_CALL) {
        comp_expression(rhs, a);
        comp_expression(lhs, a);
    } else {
        comp_expression(lhs, a);
        comp_expression(rhs, a);
    }

    switch (op) {
        case AST_OPERATOR_CALL: emit_call(a.emitter); break;

        case AST_OPERATOR_EQ: emit_eqi(a.emitter); break;
        case AST_OPERATOR_NE: emit_nei(a.emitter); break;
        case AST_OPERATOR_LE: emit_lei(a.emitter); break;
        case AST_OPERATOR_LT: emit_lti(a.emitter); break;
        case AST_OPERATOR_GE: emit_gei(a.emitter); break;
        case AST_OPERATOR_GT: emit_gti(a.emitter); break;

        case AST_OPERATOR_ADD: emit_addi(a.emitter); break;
        case AST_OPERATOR_SUB: emit_subi(a.emitter); break;
        case AST_OPERATOR_MUL: emit_muli(a.emitter); break;
        case AST_OPERATOR_DIV: emit_divi(a.emitter); break;
        case AST_OPERATOR_REM: emit_remi(a.emitter); break;
    }
}

static void comp_expression_conditional(AstExpressionView v, Compiler a) {
    EmitterSymbol else_start = emit_symbol(a.emitter);
    EmitterSymbol conditional_end = emit_symbol(a.emitter);

    comp_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.condition), a);
    emit_jmpz(a.emitter, else_start);

    comp_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_true), a);
    emit_jmp(a.emitter, conditional_end);

    emit_symbol_location(a.emitter, else_start);
    comp_expression(AST_VIEW_CHILD_EXPRESSION(v, .as.conditional.if_false), a);
    emit_symbol_location(a.emitter, conditional_end);
}

static void comp_expression_tuple(AstExpressionView v, Compiler a) {
    for (
        AstExpressionView e = AST_VIEW_CHILD_EXPRESSION(v, .as.tuple.elements);
        e.data;
        e = AST_VIEW_CHILD_EXPRESSION(e, .next)
    ) {
        comp_expression(e, a);
    }
}

static void comp_expression_literal_int(AstExpressionView v, Compiler a) {
    emit_int(a.emitter, v.data->as.literal_int);
}

static void comp_constant(AnConstant v, Compiler a) {
    switch (v.kind) {
        case AN_CONSTANT_INVALID: break; // FIXME: check here?
        case AN_CONSTANT_FUNCTION:
            emit_loc(a.emitter, v.as.function);
            break;
        case AN_CONSTANT_TYPE:
            emit_int(a.emitter, v.as.type); // FIXME: something better than int?
            break;
        case AN_CONSTANT_TUPLE:
            for (size_t i = 0; i < v.as.tuple.len; i++) {
                comp_constant(v.as.tuple.elements[i], a);
            }
            break;
    }
}

static void comp_pattern(AstPatternView v, Compiler a) {
    switch(v.data->kind) {
        case AST_PATTERN_VARIABLE: comp_pattern_variable(v, a); break;
        case AST_PATTERN_TUPLE: comp_pattern_tuple(v, a); break;
    }
}

static void comp_pattern_variable(AstPatternView v, Compiler a) {
    AnBindingId binding_id = *(AnBindingId const*)id_hash_table_get(a.analysis.variable_bindings, v.id);
    BytecodeVarRange var_range = 
        an_get_binding(a.analysis.scope_bindings, binding_id)->as.variable.range;
    for (int32_t i = (int32_t)var_range.end - 1; i >= var_range.start; i--) {
        emit_set(a.emitter, i);
    }
}

static void comp_pattern_tuple(AstPatternView v, Compiler a) {
    DYNAMIC_ARRAY(AstPatternId) elements = {0};
    for (
        AstPatternView e = AST_VIEW_CHILD_PATTERN(v, .as.tuple.elements);
        e.data;
        e = AST_VIEW_CHILD_PATTERN(e, .next)
    ) {
        DYNAMIC_ARRAY_PUSH(&elements, e.id);
    }
    for (intptr_t i = elements.len - 1; i >= 0; i--) {
        comp_pattern(AST_VIEW_PATTERN(v.nodes, elements.data[i]), a);
    }
    free(elements.data);
}

static void comp_binding_set(AstBindingSetView v, Compiler a) {
    if (v.data->kind != AST_BINDING_SET_VARIABLE) return;

    comp_expression(AST_VIEW_CHILD_EXPRESSION(v, .value), a);
    comp_pattern(AST_VIEW_CHILD_PATTERN(v, .pattern), a);
}
