#include "phase/parse.h"
#include "phase/parse/util.h"

ParseStatus parse_binding(Parser p, OUT(AstBinding) dst) {
    TokenTree head;
    if (
        token_it_next(p.tokens, &head)
        || head.kind != TOKEN_TREE_SINGLE
        || (head.as.single.kind != TOKEN_CONST && head.as.single.kind != TOKEN_LET)
    ) {
        unexpected_token();
        return PARSE_ERROR;
    }
    AstBindingKind kind;
    switch (head.as.single.kind) {
        case TOKEN_CONST: kind = AST_BINDING_CONSTANT; break;
        case TOKEN_LET: kind = AST_BINDING_VARIABLE; break;
        default: break; // unreachable
    }

    Range range = head.as.single.range;
    AstPattern* p_pattern = NULL;
    AstExpression* p_value = NULL;
    ParseStatus status; // must be set before gotoing to parse_binding_end

    AstPattern pattern;
    ParseStatus pattern_status = parse_pattern(p, &pattern);
    if (parse_status_returned(pattern_status)) {
        p_pattern = ast_storage_alloc(p.storage, sizeof(AstPattern));
        *p_pattern = pattern;
        range.end = pattern.range.end;
    }
    switch (pattern_status) {
        case PARSE_OK:
            break;
        case PARSE_ILL:
        case PARSE_ERROR:
            status = PARSE_ILL;
            goto parse_binding_end;
    }

    Token equal;
    if (token_it_match_single(p.tokens, TOKEN_EQUAL, &equal)) {
        unexpected_token();
        status = PARSE_ILL;
        goto parse_binding_end;
    }
    range.end = equal.range.end;

    AstExpression value;
    ParseStatus value_status = parse_expression(p, &value);
    if (parse_status_returned(value_status)) {
        p_value = ast_storage_alloc(p.storage, sizeof(AstExpression));
        *p_value = value;
        range.end = value.range.end;
    }
    switch (value_status) {
        case PARSE_OK:
            break;
        case PARSE_ILL:
        case PARSE_ERROR:
            status = PARSE_ILL;
            goto parse_binding_end;
    }

    status = PARSE_OK;
    goto parse_binding_end;

parse_binding_end:
    *dst = (AstBinding) {
        .next = NULL,
        .range = range,
        .kind = kind,
        .pattern = p_pattern,
        .value = p_value,
    };
    return status;
}
