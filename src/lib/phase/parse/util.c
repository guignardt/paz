#include "phase/parse/util.h"
#include "diagnostic/report.h"

void unexpected_token(void) {
    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
    report_message("unexpected token");
    report_end();
}

AstExpressionId box_expression(Parser p, AstExpression v) {
    AstExpressionId id = p.nodes->expressions_len;
    reserve(
        ERASE2(&p.nodes->expressions),
        &p.nodes->_expressions_cap,
        p.nodes->expressions_len + 1,
        sizeof(AstExpression)
    );
    p.nodes->expressions[p.nodes->expressions_len++] = v;
    return id;
}

AstPatternId box_pattern(Parser p, AstPattern v) {
    AstPatternId id = p.nodes->patterns_len;
    reserve(
        ERASE2(&p.nodes->patterns),
        &p.nodes->_patterns_cap,
        p.nodes->patterns_len + 1,
        sizeof(AstPattern)
    );
    p.nodes->patterns[p.nodes->patterns_len++] = v;
    return id;
}

AstBindingSetId box_binding_set(Parser p, AstBindingSet v) {
    AstBindingSetId id = p.nodes->binding_sets_len;
    reserve(
        ERASE2(&p.nodes->binding_sets),
        &p.nodes->_binding_sets_cap,
        p.nodes->binding_sets_len + 1,
        sizeof(AstBindingSet)
    );
    p.nodes->binding_sets[p.nodes->binding_sets_len++] = v;
    return id;
}

