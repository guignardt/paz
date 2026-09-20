#include "phase/parse/util.h"
#include "diagnostic/report.h"

void unexpected_token(void) {
    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
    report_message("unexpected token");
    report_end();
}

AstExpressionId box_expression(Parser p, AstExpression v) {
    AstExpressionId id = p.nodes->expressions.len;
    DYNAMIC_ARRAY_PUSH(&p.nodes->expressions, v);
    return id;
}

AstPatternId box_pattern(Parser p, AstPattern v) {
    AstExpressionId id = p.nodes->patterns.len;
    DYNAMIC_ARRAY_PUSH(&p.nodes->patterns, v);
    return id;
}

AstBindingSetId box_binding_set(Parser p, AstBindingSet v) {
    AstExpressionId id = p.nodes->binding_sets.len;
    DYNAMIC_ARRAY_PUSH(&p.nodes->binding_sets, v);
    return id;
}

