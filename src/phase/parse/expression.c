#include "phase/parse.h"
#include "phase/parse/util.h"

typedef enum Precedence {
    PRECEDENCE_NONE,
    PRECEDENCE_ADD,
    PRECEDENCE_CALL,
    PRECEDENCE_ATOM,
} Precedence;

Precedence binary_operator_precedence[] = {
    [AST_OPERATOR_CALL] = PRECEDENCE_CALL,
    [AST_OPERATOR_ADD] = PRECEDENCE_ADD,
};

typedef struct TokenOperatorEntry {
    enum {
        TOKEN_OPERATOR_ENTRY_SET = 0x1,
        TOKEN_OPERATOR_ENTRY_INFIX = 0x2,
    } flags;
    int operator;
} TokenOperatorEntry;

TokenOperatorEntry token_operators[] = {
    [TOKEN_PLUS] =  { TOKEN_OPERATOR_ENTRY_SET | TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_ADD }
};

static ParseStatus parse_expression_prec(Parser p, Precedence prec, OUT(AstExpression) dst);
static ParseStatus parse_expression_head(Parser p, OUT(AstExpression) dst);
// only returns `PARSE_OK` or `PARSE_ILL`
static ParseStatus parse_expression_try_continue(
    Parser p, Precedence prec, AstExpression lhs, OUT(AstExpression) dst, OUT(bool) parsed
);

// *infix set to true if we have an infix operator, to false if we have a
// prefix operator
static int token_operator(TokenKind kind, int* dst, bool* infix);

static ParseStatus ast_ref_new(char const* text, Range identifier, OUT(AstExpression) dst);
// assumes the head `fn` token has already been consumed
static ParseStatus parse_function_continue(Parser p, Range fn_range, OUT(AstExpression) dst);

ParseStatus parse_expression(Parser p, OUT(AstExpression) dst) {
    return parse_expression_prec(p, PRECEDENCE_NONE, dst);
}

static ParseStatus parse_expression_prec(Parser p, Precedence prec, OUT(AstExpression) dst) {
    ParseStatus status;
    AstExpression expr;
    
    {
        status = parse_expression_head(p, &expr);
        if (status != PARSE_OK) {
            if (status == PARSE_ERROR) {
                return PARSE_ERROR;
            }
            goto parse_expression_end;
        }
    }

    if (prec == PRECEDENCE_ATOM) {
        goto parse_expression_end;
    }

    // status is now `PARSE_OK`

    while (true) {
        bool parsed;
        status = parse_expression_try_continue(p, prec, expr, &expr, &parsed);
        if (!parsed) {
            goto parse_expression_end;
        }
    }

parse_expression_end:
    if (dst) {
        *dst = expr;
    }
    return status;
}

static ParseStatus parse_expression_head(Parser p, OUT(AstExpression) dst) {
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
                case TOKEN_IDENTIFIER:
                    return ast_ref_new(p.text, head.as.single.range, dst);
                case TOKEN_FN:
                    return parse_function_continue(p, head.as.single.range, dst);
                default:
                    return PARSE_ERROR;
            }
    }
}

static ParseStatus parse_expression_try_continue(
    Parser p, Precedence prec, AstExpression lhs, OUT(AstExpression) dst, OUT(bool) parsed
) {
    ParseStatus status;

    TokenTree head;
    if (token_it_get(*p.tokens, &head)) {
        goto parse_expression_try_continue_no_parse;
    }

    AstBinaryOperator operator;
    bool operator_token;
    switch (head.kind) {
        case TOKEN_TREE_GROUP:
            switch (head.as.group.delim) {
                case TOKEN_DELIM_PAREN:
                    operator = AST_OPERATOR_CALL;
                    operator_token = false;
                    break;
            }
            break;
        case TOKEN_TREE_SINGLE:;
            TokenOperatorEntry entry = token_operators[head.as.single.kind];
            if (
                !(entry.flags & TOKEN_OPERATOR_ENTRY_SET)
                || !(entry.flags & TOKEN_OPERATOR_ENTRY_INFIX)
            ) {
                goto parse_expression_try_continue_no_parse;
            }
            operator = entry.operator;
            operator_token = true;
            break;
    }

    Precedence operator_prec = binary_operator_precedence[operator];
    if (operator_prec <= prec) {
        goto parse_expression_try_continue_no_parse;
    }

    if (operator_token) {
        token_it_next(p.tokens, NULL);
    }

    AstExpression rhs;
    status = parse_expression_prec(p, operator_prec, &rhs);

    AstExpression* p_lhs = ast_storage_alloc(p.storage, sizeof(AstExpression));
    *p_lhs = lhs;
    AstExpression* p_rhs;
    if (status == PARSE_ERROR) {
        status = PARSE_ILL;
        p_rhs = NULL;
    } else {
        p_rhs = ast_storage_alloc(p.storage, sizeof(AstExpression));
        *p_rhs = rhs;
    }

    AstExpression result = {
        .next = NULL,
        .range = (Range) { lhs.range.start, rhs.range.end },
        .kind = AST_EXPRESSION_BINARY_OPERATION,
        .as.binary_operation = (AstBinaryOperation) {
            .op = operator,
            .lhs = p_lhs,
            .rhs = p_rhs,
        },
    };
    if (dst) {
        *dst = result;
    }

    *parsed = true;
    return status;

parse_expression_try_continue_no_parse:
    *dst = lhs;
    *parsed = false;
    return PARSE_OK;
}

static ParseStatus ast_ref_new(char const* text, Range range, OUT(AstExpression) dst) {
    AstIdentifier identifier = {
        .range = range,
        .string = {
            .data = text + range.start,
            .len = range.end - range.start
        }
    };
    if (dst) {
        *dst = (AstExpression) {
            .next = NULL,
            .range = range,
            .kind = AST_EXPRESSION_REF,
            .as.ref = (AstRef) {
                .identifier = identifier,
            },
        };
    }
    return PARSE_OK;
}

static ParseStatus parse_function_continue(Parser p, Range range, OUT(AstExpression) dst) {
    AstFunction result = { .input = NULL, .output_type = NULL, .output = NULL };
    ParseStatus status; // must be set before gotoing to parse_function_continue_end

    {
        AstPattern input;
        ParseStatus pattern_status = parse_pattern(p, &input);
        if (parse_status_returned(pattern_status)) {
            result.input = ast_storage_alloc(p.storage, sizeof(AstPattern));
            *result.input = input;
            range.end = input.range.end;
        }
        if (parse_status_ill(pattern_status)) {
            status = PARSE_ILL;
            goto parse_function_continue_end;
        }
    }

    {
        Token arrow;
        if (token_it_match_single(p.tokens, TOKEN_ARROW, &arrow)) {
            unexpected_token();
            status = PARSE_ILL;
            goto parse_function_continue_end;
        }
        range.end = arrow.range.end;
    }
    
    {
        AstTypeName output_type;
        ParseStatus output_type_status = parse_type_name(p, &output_type);
        if (parse_status_returned(output_type_status)) {
            result.output_type = ast_storage_alloc(p.storage, sizeof(AstTypeName));
            *result.output_type = output_type;
            range.end = output_type.range.end;
        }
        if (parse_status_ill(output_type_status)) {
            status = PARSE_ILL;
            goto parse_function_continue_end;
        }
    }

    {
        Token double_arrow;
        if (token_it_match_single(p.tokens, TOKEN_DOUBLE_ARROW, &double_arrow)) {
            unexpected_token();
            status = PARSE_ILL;
            goto parse_function_continue_end;
        }
        range.end = double_arrow.range.end;
    }

    {
        AstExpression output;
        ParseStatus output_status = parse_expression(p, &output);
        if (parse_status_returned(output_status)) {
            result.output = ast_storage_alloc(p.storage, sizeof(AstExpression));
            *result.output = output;
            range.end = output.range.end;
        }
        if (parse_status_ill(output_status)) {
            status = PARSE_ILL;
            goto parse_function_continue_end;
        }
    }

    status = PARSE_OK;
    goto parse_function_continue_end;

parse_function_continue_end:
    if (dst) {
        *dst = (AstExpression) {
            .next = NULL,
            .range = range,
            .kind = AST_EXPRESSION_FUNCTION,
            .as.function = result,
        };
    }
    return status;
}
