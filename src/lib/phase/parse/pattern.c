#include "phase/parse.h"
#include "phase/parse/util.h"

ParseStatus parse_pattern(Parser p, OUT(AstPatternId) dst) {
    TokenTree head;
    if (token_it_next(p.tokens, &head)) {
        unexpected_token();
        if (dst) *dst = ID_SENTINEL;
        return PARSE_ILL;
    }

    switch (head.kind) {
        case TOKEN_TREE_GROUP:
            switch (head.as.group.delim) {
                case TOKEN_DELIM_PAREN:;
                    TokenIt contents = token_group_contents(head.as.group);
                    Parser p2 = {
                        .text = p.text,
                        .tokens = &contents,
                        .nodes = p.nodes,
                    };
                    return parse_pattern(p2, dst);
            }
        case TOKEN_TREE_SINGLE:
            switch (head.as.single.kind) {
                case TOKEN_IDENTIFIER: break;
                default:
                    if (dst) *dst = ID_SENTINEL;
                    return PARSE_ILL;
            }
            break;
    }

    // head is identifier

    Range range = head.as.single.range;
    AstIdentifier identifier = {
        .range = range,
        .string = {
            .data = p.text + range.start,
            .len = range.end - range.start
        }
    };

    AstExpressionId type_bound = ID_SENTINEL;
    ParseStatus type_status = PARSE_OK;
    Token colon_token;
    if (!token_it_match_single(p.tokens, TOKEN_COLON, &colon_token)) {
        range.end = colon_token.range.end;
        type_status = parse_expression(p, &type_bound);
        if (type_bound) {
            range.end = p.nodes->expressions[type_bound].range.end;
        }
    }

    AstPatternId v = box_pattern(p, (AstPattern) {
        .next = ID_SENTINEL,
        .range = range,
        .kind = AST_PATTERN_VARIABLE,
        .as.variable = (AstPatternVariable) {
            .identifier = identifier,
            .type_bound = type_bound,
        },
    });
    if (dst) *dst = v;
    return type_status;
}
