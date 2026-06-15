#include <string.h>
#include <ctype.h>

#include "diagnostic/report.h"
#include "phase/tokenize.h"

typedef struct Tokenizer {
    char const* text;
    size_t pos;
    TokenStream* dst;
} Tokenizer;

static void skip_whitespace(Tokenizer* tokenizer);
// -1 for no delimiter
static void tokenize_group(Tokenizer* tokenizer, TokenDelim delim);
static void tokenize_one(Tokenizer* tokenizer);
static void tokenize_identifier_or_keyword(Tokenizer* tokenizer);
static void tokenize_punctuation(Tokenizer* tokenizer);

void tokenize(char const* text, OUT(TokenStream) dst) {
    *dst = token_stream_new();
    Tokenizer tokenizer = {
        .text = text,
        .pos = 0,
        .dst = dst
    };
    return tokenize_group(&tokenizer, -1);
}

static void skip_whitespace(Tokenizer* tokenizer) {
    char c;
    for (; c = tokenizer->text[tokenizer->pos], c != '\0'; tokenizer->pos++) {
        if (!isspace(c)) {
            break;
        }
    }
}

static void tokenize_group(Tokenizer* tokenizer, TokenDelim delim) {
    // FIXME: detect mismatched delimiters better

    char right;
    char const* right_description;
    TokenStreamGroupBuilder group;
    if (delim == -1) {
        right = '\0';
        right_description = "end-of-file";
    } else {
        TokenDelimInfo info = token_delim_info(delim);
        right = info.right;
        right_description = info.right_description;
        group = token_stream_open_group(tokenizer->dst, TOKEN_DELIM_PAREN, tokenizer->pos++);
    }

    while (true) {
        skip_whitespace(tokenizer);
        char c = tokenizer->text[tokenizer->pos];
        if (c == right) {
            if (delim != -1) {
                token_stream_close_group(&group, tokenizer->pos++);
            }
            break;
        }
        if (c == '\0') {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("unexpected end-of-file");
            report_end();
            break;
        }

        tokenize_one(tokenizer);
    }
}

static void tokenize_one(Tokenizer* tokenizer) {
    char c = tokenizer->text[tokenizer->pos];
    if (c == '_' || isalpha(c)) {
        tokenize_identifier_or_keyword(tokenizer);
    } else if (c == '(') {
        tokenize_group(tokenizer, TOKEN_DELIM_PAREN);
    } else {
        tokenize_punctuation(tokenizer);
    }
}

static void tokenize_identifier_or_keyword(Tokenizer* tokenizer) {
    size_t start = tokenizer->pos;
    char c;
    for (; c = tokenizer->text[tokenizer->pos], c != '\0'; tokenizer->pos++) {
        if (c == '_' || isalnum(c)) {
            continue;
        }
        break;
    }
    Token token = {
        .kind = TOKEN_IDENTIFIER,
        .range = (Range) { start, tokenizer->pos },
    };
    token_stream_push(tokenizer->dst, token);
}

static void tokenize_punctuation(Tokenizer* tokenizer) {
    Token token;
    size_t len = 0;
    for (TokenKind kind = TOKEN_PUNCT_MIN; kind <= TOKEN_PUNCT_MAX; kind++) {
        TokenKindInfo info = token_kind_info(kind);
        if (info.len < len) {
            continue;
        }
        if (!strncmp(info.exact_chars, tokenizer->text + tokenizer->pos, info.len)) {
            token = (Token) {
                .kind = kind,
                .range = { tokenizer->pos, tokenizer->pos + info.len },
            };
            len = info.len;
        }
    }

    if (len == 0) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("unexpected character");
        report_end();
        return;
    }

    token_stream_push(tokenizer->dst, token);
}
