#include "phase/parse.h"
#include "phase/parse/util.h"

#include "diagnostic/report.h"

ParseStatus parse_binding_set(Parser p, OUT(AstBindingSetId) dst) {
    TokenTree head;
    if (
        token_it_next(p.tokens, &head)
        || head.kind != TOKEN_TREE_SINGLE
        || (head.as.single.kind != TOKEN_CONST && head.as.single.kind != TOKEN_LET)
    ) {
        unexpected_token();
        if (dst) *dst = ID_SENTINEL;
        return PARSE_ILL;
    }
    AstBindingSetKind kind;
    switch (head.as.single.kind) {
        case TOKEN_CONST: kind = AST_BINDING_SET_CONSTANT; break;
        case TOKEN_LET: kind = AST_BINDING_SET_VARIABLE; break;
        default: __builtin_unreachable(); break; // see check above
    }

    // from here, we always return something, even if parsing failed further.

    AstBindingSet v = {
        .next = ID_SENTINEL,
        .range = head.as.single.range,
        .kind = kind,
        .pattern = ID_SENTINEL,
        .value = ID_SENTINEL,
    };
    ParseStatus status = PARSE_ILL;

    {
        if (parse_pattern(p, &v.pattern)) goto parse_binding_end;
        if (v.pattern != ID_SENTINEL) {
            v.range.end = p.nodes->patterns.data[v.pattern].range.end;
        }
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
        if (v.value != ID_SENTINEL) {
            v.range.end = p.nodes->expressions.data[v.value].range.end;
        }
    }

    status = PARSE_OK;
    goto parse_binding_end;

parse_binding_end:;
    AstBindingSetId id = box_binding_set(p, v);
    if (dst) *dst = id;
    return status;
}
