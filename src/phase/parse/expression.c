#include "phase/parse.h"
#include "phase/parse/util.h"

ParseStatus parse_expression(Parser p, OUT(AstExpression) dst) {
    TokenTree head;
    if (token_it_next(p.tokens, &head)) {
        unexpected_token();
        return PARSE_ERROR;
    }

    switch (head.kind) {
        case TOKEN_TREE_GROUP:
            switch (head.as.group.delim) {
                case TOKEN_DELIM_PAREN:;
                    TokenIt contents = token_group_contents(head.as.group);
                    Parser p2 = {
                        .text = p.text,
                        .tokens = &contents,
                        .storage = p.storage,
                    };
                    return parse_expression(p2, dst);
            }
        case TOKEN_TREE_SINGLE:
            switch (head.as.single.kind) {
                case TOKEN_IDENTIFIER: break;
                default: return PARSE_ERROR;
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
    *dst = (AstExpression) {
        .next = NULL,
        .range = range,
        .kind = AST_EXPRESSION_REF,
        .as.ref = (AstRef) {
            .identifier = identifier,
        },
    };
    return PARSE_OK;
}
