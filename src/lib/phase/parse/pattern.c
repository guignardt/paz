#include "phase/parse.h"
#include "phase/parse/util.h"

static void parse_pat_tuple(Parser p, Range range, OUT(AstPatternId) dst);

ParseStatus parse_pattern(Parser p, OUT(AstPatternId) dst) {
    TokenTree head;
    if (token_it_next(p.tokens, &head)) {
        unexpected_token();
        if (dst) *dst = ID_SENTINEL;
        return PARSE_ILL;
    }

    AstPatternId id;
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
                    parse_pat_tuple(
                        p2,
                        (Range){ head.as.group.left.start, head.as.group.right.end },
                        &id
                    );
                    break;
            }
            break;

        case TOKEN_TREE_SINGLE:
            switch (head.as.single.kind) {
                case TOKEN_IDENTIFIER:;
                    AstPattern v = {
                        .next = ID_SENTINEL,
                        .range = head.as.single.range,
                        .kind = AST_PATTERN_VARIABLE,
                        .as.variable = (AstPatternVariable) {
                            .identifier = (AstIdentifier) {
                                .range = head.as.single.range,
                                .string = (String) {
                                    .data = p.text + head.as.single.range.start,
                                    .len = head.as.single.range.end - head.as.single.range.start,
                                },
                            },
                        },
                        .type_bound = ID_SENTINEL,
                    };
                    id = box_pattern(p, v);
                    break;

                default:
                    if (dst) *dst = ID_SENTINEL;
                    return PARSE_ILL;
            }
            break;
    }

    AstPattern* pat = &p.nodes->patterns.data[id];

    AstExpressionId type_bound = ID_SENTINEL;
    ParseStatus type_status = PARSE_OK;
    Token colon_token;
    if (!token_it_match_single(p.tokens, TOKEN_COLON, &colon_token)) {
        pat->range.end = colon_token.range.end;
        type_status = parse_expression(p, &type_bound);
        if (type_bound != ID_SENTINEL) {
            pat->type_bound = type_bound;
            pat->range.end = p.nodes->expressions.data[type_bound].range.end;
        }
    }
    
    if (dst) *dst = id;
    return type_status;
}

static void parse_pat_tuple(Parser p, Range range, OUT(AstPatternId) dst) {
    AstPatternTuple tuple = { .elements = ID_SENTINEL };
    AstPatternId last_element = ID_SENTINEL;
    bool parsed_comma = false;
    while (true) {
        if (token_it_end(*p.tokens)) {
            break;
        }

        AstPatternId elt;
        bool ill = parse_pattern(p, &elt);
        if (!ill ) {
            bool parsed_comma2 = !token_it_match_single(p.tokens, TOKEN_COMMA, NULL);
            parsed_comma |= parsed_comma2;
            if (!parsed_comma2 && !token_it_end(*p.tokens)) {
                unexpected_token();
                ill = true;
            }
        }
        if (ill) {
            TokenTree token;
            while (!token_it_next(p.tokens, &token)) {
                if (token.kind == TOKEN_TREE_SINGLE && token.as.single.kind == TOKEN_COMMA) {
                    parsed_comma = true;
                    break;
                }
            }
            continue;
        }

        if (elt == ID_SENTINEL) {
            continue;
        }

        if (tuple.elements == ID_SENTINEL) {
            tuple.elements = elt;
            last_element = elt;
        } else {
            p.nodes->patterns.data[last_element].next = elt;
            last_element = elt;
        }
    }

    AstPatternId id;
    if (tuple.elements != ID_SENTINEL && !parsed_comma) {
        id = tuple.elements;
    } else {
        AstPattern v = {
            .next = ID_SENTINEL,
            .range = range,
            .kind = AST_PATTERN_TUPLE,
            .as.tuple = tuple,
            .type_bound = ID_SENTINEL,
        };
        id = box_pattern(p, v);
    }
    
    if (dst) *dst = id;
}
