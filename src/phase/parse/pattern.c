#include "phase/parse.h"
#include "phase/parse/util.h"

ParseStatus parse_pattern(Parser p, OUT(AstPattern*) dst) {
    TokenTree head;
    if (token_it_next(p.tokens, &head)) {
        unexpected_token();
        if (dst) *dst = NULL;
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
                        .storage = p.storage,
                    };
                    return parse_pattern(p2, dst);
            }
        case TOKEN_TREE_SINGLE:
            switch (head.as.single.kind) {
                case TOKEN_IDENTIFIER: break;
                default:
                    if (dst) *dst = NULL;
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

    AstTypeName* type_name = NULL;
    ParseStatus type_status = PARSE_OK;
    Token colon_token;
    if (!token_it_match_single(p.tokens, TOKEN_COLON, &colon_token)) {
        range.end = colon_token.range.end;
        type_status = parse_type_name(p, &type_name);
        if (type_name) {
            range.end = type_name->range.end;
        }
    }

    AstPattern pat = {
        .next = NULL,
        .range = range,
        .kind = AST_PATTERN_VARIABLE,
        .as.variable = (AstPatternVariable) {
            .identifier = identifier,
            .type_name = type_name,
        },
    };
    AstPattern* ptr = ast_storage_alloc(p.storage, sizeof(AstPattern));
    *ptr = pat;
    if (dst) *dst = ptr;
    return type_status;
}

ParseStatus parse_type_name(Parser p, OUT(AstTypeName*) dst) {
    Token identifier_token;
    if (token_it_match_single(p.tokens, TOKEN_IDENTIFIER, &identifier_token)) {
        *dst = NULL;
        return PARSE_ILL;
    }
    Range range = identifier_token.range;
    AstIdentifier identifier = {
        .range = range,
        .string = {
            .data = p.text + range.start,
            .len = range.end - range.start
        }
    };
    AstTypeName v = {
        .next = NULL,
        .range = range,
        .kind = AST_TYPE_NAME_IDENTIFIER,
        .as.identifier = identifier,
    };
    AstTypeName* ptr = ast_storage_alloc(p.storage, sizeof(AstTypeName));
    *ptr = v;
    if (dst) *dst = ptr;
    return PARSE_OK;
}
