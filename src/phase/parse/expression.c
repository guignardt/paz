#include "phase/parse.h"

ParseStatus parse_expression(Parser p, OUT(AstExpression) dst) {
    Token identifier_token;
    if (token_it_match_single(p.tokens, TOKEN_IDENTIFIER, &identifier_token)) {
        return PARSE_ERROR;
    }
    Range range = identifier_token.range;
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
