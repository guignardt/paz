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

    TOKEN_IDENTIFIER,
} TokenKind;

#define TOKEN_PUNCT_MIN (TOKEN_COMMA)
#define TOKEN_PUNCT_MAX (TOKEN_COMMA)

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

int token_it_get(TokenIt it, OUT(TokenTree) dst);
int token_it_next(TokenIt* it, OUT(TokenTree) dst);

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
