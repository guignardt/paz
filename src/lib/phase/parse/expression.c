#include "phase/parse.h"
#include "phase/parse/util.h"

#include "diagnostic/report.h"

typedef enum Precedence {
    PRECEDENCE_NONE,
    PRECEDENCE_FLOW,
    PRECEDENCE_CONDITIONAL,
    PRECEDENCE_FOR,
    PRECEDENCE_COMPARE,
    PRECEDENCE_ADD,
    PRECEDENCE_MUL,
    PRECEDENCE_CALL,
    PRECEDENCE_ATOM,
} Precedence;

Precedence unary_operator_precedence[] = {
    [AST_OPERATOR_BREAK] = PRECEDENCE_FLOW,
    [AST_OPERATOR_CONTINUE] = PRECEDENCE_FLOW,
};

Precedence binary_operator_precedence[] = {
    [AST_OPERATOR_CALL] = PRECEDENCE_CALL,

    [AST_OPERATOR_EQ] = PRECEDENCE_COMPARE,
    [AST_OPERATOR_NE] = PRECEDENCE_COMPARE,
    [AST_OPERATOR_LE] = PRECEDENCE_COMPARE,
    [AST_OPERATOR_LT] = PRECEDENCE_COMPARE,
    [AST_OPERATOR_GE] = PRECEDENCE_COMPARE,
    [AST_OPERATOR_GT] = PRECEDENCE_COMPARE,
    
    [AST_OPERATOR_ADD] = PRECEDENCE_ADD,
    [AST_OPERATOR_SUB] = PRECEDENCE_ADD,
    [AST_OPERATOR_MUL] = PRECEDENCE_MUL,
    [AST_OPERATOR_DIV] = PRECEDENCE_MUL,
    [AST_OPERATOR_REM] = PRECEDENCE_MUL,
};

typedef struct TokenOperatorEntry {
    enum {
        TOKEN_OPERATOR_ENTRY_PREFIX = 0x1,
        TOKEN_OPERATOR_ENTRY_INFIX = 0x2,
    } flags;
    int operator;
} TokenOperatorEntry;

TokenOperatorEntry token_operators[] = {
    [TOKEN_BREAK] =         { TOKEN_OPERATOR_ENTRY_PREFIX, AST_OPERATOR_BREAK },
    [TOKEN_CONTINUE] =      { TOKEN_OPERATOR_ENTRY_PREFIX, AST_OPERATOR_CONTINUE },

    [TOKEN_EQUAL_EQUAL] =   { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_EQ },
    [TOKEN_BANG_EQUAL] =    { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_NE },
    [TOKEN_LESS_EQUAL] =    { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_LE },
    [TOKEN_LESS] =          { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_LT },
    [TOKEN_GREATER_EQUAL] = { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_GE },
    [TOKEN_GREATER] =       { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_GT },

    [TOKEN_PLUS] =          { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_ADD },
    [TOKEN_MINUS] =         { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_SUB },
    [TOKEN_STAR] =          { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_MUL },
    [TOKEN_SLASH] =         { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_DIV },
    [TOKEN_PERCENT] =       { TOKEN_OPERATOR_ENTRY_INFIX, AST_OPERATOR_REM },

    [TOKEN_IDENTIFIER] = {0} // ensure every token kind has a slot
};

static ParseStatus parse_expression_prec(Parser p, Precedence prec, OUT(AstExpressionId) dst);
static ParseStatus parse_expression_primary(Parser p, OUT(AstExpressionId) dst);
// only returns `PARSE_OK` or `PARSE_ILL`
static ParseStatus parse_expression_try_continue(
    Parser p, Precedence prec, AstExpressionId lhs, OUT(AstExpressionId) dst, OUT(bool) parsed
);

// takes in the contents of the parenthesized expression
static ParseStatus parse_expression_paren(Parser p, Range range, OUT(AstExpressionId) dst);
static void ast_ref_new(Parser p, Range identifier, OUT(AstExpressionId) dst);
// assumes the head `fn` token has already been consumed
static ParseStatus parse_function_continue(Parser p, Range fn_range, OUT(AstExpressionId) dst);
// assumes the head `if` token has already been consumed
static ParseStatus parse_conditonal_continue(Parser p, Range if_range, OUT(AstExpressionId) dst);
// doesn't actually read from the parser
static ParseStatus parse_int(Parser p, Token token, OUT(AstExpressionId) dst);

ParseStatus parse_expression(Parser p, OUT(AstExpressionId) dst) {
    return parse_expression_prec(p, PRECEDENCE_NONE, dst);
}

static ParseStatus parse_expression_prec(Parser p, Precedence prec, OUT(AstExpressionId) dst) {
    ParseStatus status = PARSE_ILL;
    AstExpressionId expr;
    
    // parse head
    if (parse_expression_primary(p, &expr)) goto parse_expression_end;

    if (prec == PRECEDENCE_ATOM) goto parse_expression_end;

    // status is now `PARSE_OK`

    while (true) {
        bool parsed;
        status = parse_expression_try_continue(p, prec, expr, &expr, &parsed);
        if (!parsed || status) {
            goto parse_expression_end;
        }
    }

parse_expression_end:
    if (dst) *dst = expr;
    return status;
}

static ParseStatus parse_expression_primary(Parser p, OUT(AstExpressionId) dst) {
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
                    return parse_expression_paren(
                        p2,
                        (Range){ head.as.group.left.start, head.as.group.right.end },
                        dst
                    );
            }

        case TOKEN_TREE_SINGLE:;
            TokenOperatorEntry operator_entry = token_operators[head.as.single.kind];
            if (operator_entry.flags & TOKEN_OPERATOR_ENTRY_PREFIX) {
                Precedence prec = unary_operator_precedence[operator_entry.operator];
                AstExpressionId operand;
                ParseStatus status = parse_expression_prec(p, prec, &operand);
                AstExpressionId operand_id = ID_SENTINEL;
                Range range = head.as.single.range;
                AstExpression v = {
                    .next = ID_SENTINEL,
                    .range = range,
                    .kind = AST_EXPRESSION_UNARY_OPERATION,
                    .as.unary_operation = (AstUnaryOperation) {
                        .operator = operator_entry.operator,
                        .operand = operand_id,
                    },
                };
                AstExpressionId id = box_expression(p, v);
                if (dst) *dst = id;
                return status;
            }

            switch (head.as.single.kind) {
                case TOKEN_IDENTIFIER:
                    ast_ref_new(p, head.as.single.range, dst);
                    return PARSE_OK;
                case TOKEN_FN:
                    return parse_function_continue(p, head.as.single.range, dst);
                case TOKEN_IF:
                    return parse_conditonal_continue(p, head.as.single.range, dst);
                case TOKEN_INT:
                    return parse_int(p, head.as.single, dst);
                default:
                    if (dst) *dst = ID_SENTINEL;
                    return PARSE_ILL;
            }
    }
}

static ParseStatus parse_expression_try_continue(
    Parser p, Precedence prec, AstExpressionId lhs, OUT(AstExpressionId) dst, OUT(bool) parsed
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
            if (!(entry.flags & TOKEN_OPERATOR_ENTRY_INFIX)) {
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

    AstExpressionId rhs;
    status = parse_expression_prec(p, operator_prec, &rhs);

    AstExpression v = {
        .next = ID_SENTINEL,
        .range = (Range) {
            p.nodes->expressions[lhs].range.start,
            p.nodes->expressions[rhs].range.end
        },
        .kind = AST_EXPRESSION_BINARY_OPERATION,
        .as.binary_operation = (AstBinaryOperation) {
            .operator = operator,
            .lhs = lhs,
            .rhs = rhs,
        },
    };
    AstExpressionId id = box_expression(p, v);
    if (dst) *dst = id;

    *parsed = true;
    return status;

parse_expression_try_continue_no_parse:
    *dst = lhs;
    *parsed = false;
    return PARSE_OK;
}

static void ast_ref_new(Parser p, Range range, OUT(AstExpressionId) dst) {
    AstIdentifier identifier = {
        .range = range,
        .string = {
            .data = p.text + range.start,
            .len = range.end - range.start
        }
    };
    AstExpression v = {
        .next = ID_SENTINEL,
        .range = range,
        .kind = AST_EXPRESSION_REFERENCE,
        .as.reference = (AstReference) {
            .identifier = identifier,
        },
    };
    AstExpressionId id = box_expression(p, v);
    if (dst) *dst = id;
}

static ParseStatus parse_expression_paren(Parser p, Range range, OUT(AstExpressionId) dst) {
    AstBindingSetId binding_sets = ID_SENTINEL;
    AstExpressionId tail = ID_SENTINEL;
    
    // parse bindings
    TokenTree head;
    AstBindingSetId previous = ID_SENTINEL;
    while (!token_it_get(*p.tokens, &head)) {
        if (head.kind != TOKEN_TREE_SINGLE) {
            break;
        }

        if (head.as.single.kind == TOKEN_SEMICOLON) {
            token_it_next(p.tokens, NULL);
            continue;
        }

        if (head.as.single.kind != TOKEN_CONST && head.as.single.kind != TOKEN_LET) {
            break;
        }
        AstBindingSetId binding;
        ParseStatus status = parse_binding_set(p, &binding);
        if (binding != ID_SENTINEL) {
            // *write_binding_set_ids = binding_set_id;
            // write_binding_set_ids = &p.nodes->binding_sets[binding_set_id].next;
            if (previous == ID_SENTINEL) {
                binding_sets = binding;
            } else {
                p.nodes->binding_sets[previous].next = binding;
            }
        }
        previous = binding;

        if (token_it_match_single(p.tokens, TOKEN_SEMICOLON, NULL)) {
            // if status is not ok, we have already reported an error when
            // parsing the binding
            if (status == PARSE_OK) {
                unexpected_token();
            }
            token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
        }
    }

    if (!token_it_end(*p.tokens)) {
        ParseStatus status = parse_expression(p, &tail);
        if (!status && !token_it_end(*p.tokens)) {
            unexpected_token();
        }
    }

    AstExpressionId id;
    if (!binding_sets) id = tail;
    else {
        AstExpression block = {
            .next = ID_SENTINEL,
            .range = range,
            .kind = AST_EXPRESSION_BLOCK,
            .as.block = (AstBlock) {
                .binding_sets = binding_sets,
                .tail = tail,
            },
        };
        AstExpressionId id = box_expression(p, block);
        if (dst) *dst = id;
    }
    if (dst) *dst = id;
    return PARSE_OK;
}

static ParseStatus parse_function_continue(Parser p, Range range, OUT(AstExpressionId) dst) {
    // FIXME: better recovering

    AstFunction v = { .input = ID_SENTINEL, .output_type = ID_SENTINEL, .output = ID_SENTINEL };
    ParseStatus status = PARSE_ILL;

    {
        if (parse_pattern(p, &v.input)) goto parse_function_continue_end;
        if (v.input) range.end = p.nodes->patterns[v.input].range.end;
    }

    {
        Token arrow;
        if (!token_it_match_single(p.tokens, TOKEN_ARROW, &arrow)) {
            range.end = arrow.range.end;
            if (parse_expression(p, &v.output_type)) goto parse_function_continue_end;
            if (v.output_type) range.end = p.nodes->expressions[v.output_type].range.end;
        }
    }

    {
        Token double_arrow;
        if (token_it_match_single(p.tokens, TOKEN_DOUBLE_ARROW, &double_arrow)) {
            unexpected_token();
            goto parse_function_continue_end;
        }
        range.end = double_arrow.range.end;
    }

    {
        if (parse_expression(p, &v.output)) goto parse_function_continue_end;
        if (v.output) range.end = p.nodes->expressions[v.output].range.end;
    }

    status = PARSE_OK;
    goto parse_function_continue_end;

parse_function_continue_end:;
    AstExpression expr = {
        .next = ID_SENTINEL,
        .range = range,
        .kind = AST_EXPRESSION_FUNCTION,
        .as.function = v,
    };
    AstExpressionId id = box_expression(p, expr);
    if (dst) *dst = id;
    return status;
}

static ParseStatus parse_conditonal_continue(Parser p, Range range, OUT(AstExpressionId) dst) {
    AstConditional v = { .condition = ID_SENTINEL, .if_true = ID_SENTINEL, .if_false = ID_SENTINEL };
    ParseStatus status = PARSE_ILL;
    
    {
        if (parse_expression(p, &v.condition)) goto parse_conditonal_continue_end;
        if (v.condition) range.end = p.nodes->expressions[v.condition].range.end;
    }

    {
        Token then;
        if (token_it_match_single(p.tokens, TOKEN_THEN, &then)) {
            goto parse_conditonal_continue_end;
        }
        range.end = then.range.end;
    }

    {
        if (parse_expression(p, &v.if_true)) goto parse_conditonal_continue_end;
        if (v.if_true) range.end = p.nodes->expressions[v.if_true].range.end;
    }

    {
        Token else_;
        if (token_it_match_single(p.tokens, TOKEN_ELSE, &else_)) {
            goto parse_conditonal_continue_end;
        }
        range.end = else_.range.end;
    }

    {
        if (parse_expression(p, &v.if_false)) goto parse_conditonal_continue_end;
        if (v.if_false) range.end = p.nodes->expressions[v.if_false].range.end;
    }

    status = PARSE_OK;
    goto parse_conditonal_continue_end;

parse_conditonal_continue_end:;
    AstExpression expr = {
        .next = ID_SENTINEL,
        .range = range,
        .kind = AST_EXPRESSION_CONDITIONAL,
        .as.conditional = v,
    };
    AstExpressionId id = box_expression(p, expr);
    if (dst) *dst = id;
    return status;
}

static ParseStatus parse_int(Parser p, Token token, OUT(AstExpressionId) dst) {
    size_t pos = token.range.start;

    int sign;
    switch (p.text[pos]) {
        case '+': sign = 1; pos++; break;
        case '-': sign = -1; pos++; break;
        default: sign = 1; break;
    }

    int64_t value = 0;
    for (; pos < token.range.end; pos++) {
        // must be a digit
        char c = p.text[pos];
        int64_t digit = sign * (c - '0');
        if (
            __builtin_mul_overflow(value, 10, &value)
            || __builtin_add_overflow(value, digit, &value)
        ) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("integer literal overflowed");
            report_end();
            value = 0;
            break;
        }
    }

    AstExpression v = {
        .next = ID_SENTINEL,
        .range = token.range,
        .kind = AST_EXPRESSION_LITERAL_INT,
        .as.literal_int = value,
    };
    AstExpressionId id = box_expression(p, v);
    if (dst) *dst = id;
    return PARSE_OK;
}
