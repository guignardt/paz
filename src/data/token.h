#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "util/alloc.h"

typedef struct TokenStream {
    BOXED(void) _elements;
    size_t _len;
    size_t _capacity;
} TokenStream;

typedef struct TokenIt {
    void const* _cursor;
    void const* _end;
} TokenIt;

typedef enum TokenKind {
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COLON_EQUAL,
    TOKEN_COLON_COLON,
    TOKEN_EQUAL,
    TOKEN_ARROW,
    TOKEN_DOUBLE_ARROW,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,

    TOKEN_CONST,
    TOKEN_LET,
    TOKEN_FN,

    TOKEN_IDENTIFIER,

    TOKEN_INT,
} TokenKind;

#define TOKEN_PUNCT_MIN (TOKEN_COMMA)
#define TOKEN_PUNCT_MAX (TOKEN_PERCENT)
#define TOKEN_KEYWORD_MIN (TOKEN_CONST)
#define TOKEN_KEYWORD_MAX (TOKEN_FN)

typedef struct TokenKindInfo {
    size_t len;                 // `-1` for dynamic tokens
    char exact_chars[8];        // all `0` for dynamic tokens
    char const* description;
} TokenKindInfo;

typedef struct Token {
    TokenKind kind;
    Range range;
} Token;

typedef enum TokenDelim {
    TOKEN_DELIM_PAREN,
} TokenDelim;

typedef struct TokenDelimInfo {
    char left;
    char const* left_description;
    char right;
    char const* right_description;
} TokenDelimInfo;

typedef struct TokenGroup {
    TokenDelim delim;
    Range left;
    Range right;
    TokenIt _contents;
} TokenGroup;

typedef enum TokenTreeKind {
    TOKEN_TREE_SINGLE,
    TOKEN_TREE_GROUP,
} TokenTreeKind;

typedef struct TokenTree {
    TokenTreeKind kind;
    union {
        Token single;
        TokenGroup group;
    } as;
} TokenTree;

TokenKindInfo token_kind_info(TokenKind kind);
TokenDelimInfo token_delim_info(TokenDelim delim);

TokenIt token_stream_it(TokenStream stream);

bool token_it_end(TokenIt it);
// all return nonzero if didn't return
int token_it_get(TokenIt it, OUT(TokenTree) dst);
int token_it_next(TokenIt* it, OUT(TokenTree) dst);
int token_it_match_single(TokenIt* it, TokenKind kind, OUT(Token) dst);
int token_it_match_group(TokenIt* it, TokenDelim delim, OUT(TokenGroup) dst);
int token_it_find_single(TokenIt* it, TokenKind kind, OUT(Token) dst);
int token_it_find_group(TokenIt* it, TokenDelim delim, OUT(TokenGroup) dst);

TokenIt token_group_contents(TokenGroup group);

TokenStream token_stream_new(void);

void token_stream_push(TokenStream* stream, Token token);

typedef struct TokenStreamGroupBuilder {
    TokenStream* _stream;
    size_t _opening;
} TokenStreamGroupBuilder;

TokenStreamGroupBuilder token_stream_open_group(
    TokenStream* stream,
    TokenDelim delim,
    size_t start_pos
);

void token_stream_close_group(
    OWNED(TokenStreamGroupBuilder) builder,
    size_t start_pos
);

void token_stream_free(OWNED(TokenStream) stream);

void debug_tokens(TokenIt tokens);
