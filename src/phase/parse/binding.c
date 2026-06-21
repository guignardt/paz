#include "phase/parse.h"
#include "phase/parse/util.h"

#include "diagnostic/report.h"

ParseStatus parse_binding(Parser p, OUT(AstBinding*) dst) {
    TokenTree head;
    if (
        token_it_next(p.tokens, &head)
        || head.kind != TOKEN_TREE_SINGLE
        || (head.as.single.kind != TOKEN_CONST && head.as.single.kind != TOKEN_LET)
    ) {
        unexpected_token();
        if (dst) *dst = NULL;
        return PARSE_ILL;
    }
    AstBindingKind kind;
    switch (head.as.single.kind) {
        case TOKEN_CONST: kind = AST_BINDING_CONSTANT; break;
        case TOKEN_LET: kind = AST_BINDING_VARIABLE; break;
        default: __builtin_unreachable(); break; // see check above
    }

    // from here, we always return something, even if parsing failed further.

    AstBinding v = {
        .next = NULL,
        .range = head.as.single.range,
        .kind = kind,
        .pattern = NULL,
        .value = NULL,
    };
    ParseStatus status = PARSE_ILL;

    {
        if (parse_pattern(p, &v.pattern)) goto parse_binding_end;
        if (v.pattern) v.range.end = v.pattern->range.end;
    }

    {
        Token equal;
        if (token_it_match_single(p.tokens, TOKEN_EQUAL, &equal)) {
            unexpected_token();
            goto parse_binding_end;
        }
        v.range.end = equal.range.end;
    }

    {
        if (parse_expression(p, &v.value)) goto parse_binding_end;
        if (v.value) v.range.end = v.value->range.end;
    }

    status = PARSE_OK;
    goto parse_binding_end;

parse_binding_end:;
    AstBinding* ptr = ast_storage_alloc(p.storage, sizeof(AstBinding));
    *ptr = v;
    if (dst) *dst = ptr;
    return status;
}
