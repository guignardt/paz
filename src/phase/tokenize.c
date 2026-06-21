#include <string.h>
#include <ctype.h>

#include "diagnostic/report.h"
#include "phase/tokenize.h"

typedef struct Tokenizer {
    char const* text;
    size_t pos;
    TokenStream* dst;
} Tokenizer;

static void skip_whitespace(Tokenizer* t);
// -1 for no delimiter
static void tokenize_group(Tokenizer* t, TokenDelim delim);
static void tokenize_one(Tokenizer* t);
static void tokenize_identifier_or_keyword(Tokenizer* t);
static void tokenize_punctuation(Tokenizer* t);
static void tokenize_number(Tokenizer* t);

void tokenize(char const* text, OUT(TokenStream) dst) {
    *dst = token_stream_new();
    Tokenizer tokenizer = {
        .text = text,
        .pos = 0,
        .dst = dst
    };
    return tokenize_group(&tokenizer, -1);
}

static void skip_whitespace(Tokenizer* t) {
    char c;
    for (; c = t->text[t->pos], c != '\0'; t->pos++) {
        if (!isspace(c)) {
            break;
        }
    }
}

static void tokenize_group(Tokenizer* t, TokenDelim delim) {
    // FIXME: detect mismatched delimiters better

    char right;
    TokenStreamGroupBuilder group;
    if (delim == -1) {
        right = '\0';
    } else {
        TokenDelimInfo info = token_delim_info(delim);
        right = info.right;
        group = token_stream_open_group(t->dst, TOKEN_DELIM_PAREN, t->pos++);
    }

    while (true) {
        skip_whitespace(t);
        char c = t->text[t->pos];
        if (c == right) {
            if (delim != -1) {
                token_stream_close_group(&group, t->pos++);
            }
            break;
        }
        if (c == '\0') {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("unexpected end-of-file");
            report_end();
            break;
        }

        tokenize_one(t);
    }
}

static void tokenize_one(Tokenizer* t) {
    char c = t->text[t->pos];
    if (c == '_' || isalpha(c)) {
        tokenize_identifier_or_keyword(t);
    } else if (c == '(') {
        tokenize_group(t, TOKEN_DELIM_PAREN);
    } else if (isdigit(c)) {
        tokenize_number(t);
    } else {
        tokenize_punctuation(t);
    }
}

static void tokenize_identifier_or_keyword(Tokenizer* t) {
    size_t start = t->pos;
    char c;
    for (; c = t->text[t->pos], c != '\0'; t->pos++) {
        if (c == '_' || isalnum(c)) {
            continue;
        }
        break;
    }

    for (TokenKind kind = TOKEN_KEYWORD_MIN; kind <= TOKEN_KEYWORD_MAX; kind++) {
        TokenKindInfo info = token_kind_info(kind);
        if (info.len != t->pos - start) {
            continue;
        }
        if (strncmp(t->text + start, info.exact_chars, info.len)) {
            continue;
        }

        Token token = {
            .kind = kind,
            .range = (Range) { start, t->pos },
        };
        token_stream_push(t->dst, token);
        return;
    }
    
    Token token = {
        .kind = TOKEN_IDENTIFIER,
        .range = (Range) { start, t->pos },
    };
    token_stream_push(t->dst, token);
}

static void tokenize_punctuation(Tokenizer* t) {
    Token token;
    size_t len = 0;
    for (TokenKind kind = TOKEN_PUNCT_MIN; kind <= TOKEN_PUNCT_MAX; kind++) {
        TokenKindInfo info = token_kind_info(kind);
        if (info.len < len) {
            continue;
        }
        if (!strncmp(info.exact_chars, t->text + t->pos, info.len)) {
            token = (Token) {
                .kind = kind,
                .range = { t->pos, t->pos + info.len },
            };
            len = info.len;
        }
    }

    if (len == 0) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("unexpected character");
        report_end();
        t->pos++;
        return;
    }

    bool pm = token.kind == TOKEN_PLUS || token.kind == TOKEN_MINUS;
    if (pm && isdigit(t->text[t->pos + len])) {
        tokenize_number(t);
        return;
    }

    t->pos += len;
    token_stream_push(t->dst, token);
}

static void tokenize_number(Tokenizer* t) {
    Range range = { t->pos, t->pos };

    char c = t->text[t->pos];
    if (c == '\0') {
        return;
    }

    bool pm = c == '+' || c == '-';
    Token token;
    if (pm && !isdigit(t->text[t->pos + 1])) {
        // we only have a `+` or `-`, no digits after
        TokenKind kind = (c == '+') ? TOKEN_PLUS : TOKEN_MINUS;
        range.end = ++t->pos;
        token = (Token) { .kind = kind, .range = range };
    } else {
        // we have digits
        if (pm) {
            t->pos++;
        }
        while (isdigit(t->text[t->pos])) {
            t->pos++;
        }
        range.end = t->pos;
        token = (Token) { .kind = TOKEN_INT, .range = range };
    }

    token_stream_push(t->dst, token);
}
