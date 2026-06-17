#include "phase/parse.h"

ParseStatus parse_pattern(Parser p, OUT(AstPattern) dst) {
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

    AstTypeName* p_type_name = NULL;
    ParseStatus type_status = PARSE_OK;
    Token colon_token;
    if (!token_it_match_single(p.tokens, TOKEN_COLON, &colon_token)) {
        range.end = colon_token.range.end;

        AstTypeName type_name;
        type_status = parse_type_name(p, &type_name);
        if (type_status == PARSE_ERROR) {
            p_type_name = NULL;
            type_status = PARSE_ILL;
        } else {
            range.end = type_name.range.end;
            p_type_name = ast_storage_alloc(p.storage, sizeof(AstTypeName));
            *p_type_name = type_name;
        }
    }

    if (dst) {
        *dst = (AstPattern) {
            .next = NULL,
            .range = range,
            .kind = AST_PATTERN_VARIABLE,
            .as.variable = {
                .identifier = identifier,
                .type_name = p_type_name,
            },
        };
    }
    return type_status;
}

ParseStatus parse_type_name(Parser p, OUT(AstTypeName) dst) {
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
    *dst = (AstTypeName) {
        .next = NULL,
        .range = identifier_token.range,
        .kind = AST_TYPE_NAME_IDENTIFIER,
        .as.identifier = identifier,
    };
    return PARSE_OK;
}
