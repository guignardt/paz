#include "phase/analyze.h"

#include "diagnostic/report.h"

typedef struct ConstantEvaluator {
    AnFunctionIds function_ids;
    AnScopeBindings scope_bindings;
} ConstantEvaluator;

static int evaluate_constant(ConstantEvaluator ev, AstExpression const* v, OUT(AnConstant) dst);

static int match_constant_pattern(
    AstPattern const* pattern,
    AnConstant value,
    AnScopeBindings* bindings,
    AnBindingIds binding_ids
);

void analyze_constant_bindings(Analysis* a) {
    ConstantEvaluator ev = {
        .function_ids = a->function_ids,
        .scope_bindings = a->scope_bindings,
    };

    for (size_t i = 0; i < a->constant_bindings.len; i++) {
        AstBinding const* binding = a->constant_bindings.data[i];
        
        AnConstant v = { .kind = AN_CONSTANT_INVALID };

        if (evaluate_constant(ev, binding->value, &v)) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("invalid constant binding value");
            report_end();
            continue;
        }

        if (match_constant_pattern(binding->pattern, v, &a->scope_bindings, a->binding_ids)) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("invalid constant binding value");
            report_end();
            continue;
        }
    }
}

static int evaluate_constant(ConstantEvaluator ev, AstExpression const* v, OUT(AnConstant) dst) {
    switch (v->kind) {
        case AST_EXPRESSION_FUNCTION:;
            *dst = (AnConstant) {
                .kind = AN_CONSTANT_FUNCTION,
                .as.function = *(AnFunctionId const*)ptr_hash_table_get(ev.function_ids.table, (intptr_t)v),
            };
            return 0;
        
        default:
            return -1;
    }
}

static int match_constant_pattern(
    AstPattern const* pattern,
    AnConstant value,
    AnScopeBindings* bindings,
    AnBindingIds binding_ids
) {
    switch (pattern->kind) {
        case AST_PATTERN_VARIABLE:;
            AnBindingId id = *(AnBindingId const*)ptr_hash_table_get(binding_ids.table, (intptr_t)pattern);
            AnBinding* binding = an_get_binding_mut(bindings, id);
            binding->as.constant = value;
            return 0;
    }
}
