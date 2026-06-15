#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#include "data/token.h"

static TokenKindInfo token_kind_infos[] = {
    [TOKEN_COMMA] =         {   1,  ",",    "`,`"           },
    [TOKEN_COLON] =         {   1,  ":",    "`:`"           },
    [TOKEN_SEMICOLON] =     {   1,  ";",    "`;`"           },
    [TOKEN_EQUAL] =         {   1,  "=",    "`=`"           },
    [TOKEN_ARROW] =         {   2,  "->",   "`->`"          },
    [TOKEN_DOUBLE_ARROW] =  {   2,  "=>",   "`=>`"          },
    
    [TOKEN_PLUS] =          {   1,  "+",    "`+`"           },
    [TOKEN_MINUS] =         {   1,  "-",    "`-`"           },
    [TOKEN_STAR] =          {   1,  "*",    "`*`"           },
    [TOKEN_SLASH] =         {   1,  "/",    "`/`"           },
    [TOKEN_PERCENT] =       {   1,  "%",    "`%`"           },

    [TOKEN_FN] =            {   2,  "fn",   "`fn`"          },

    [TOKEN_IDENTIFIER] =    {   -1, {0},    "<identifier>"  },
};

TokenKindInfo token_kind_info(TokenKind kind) {
    return token_kind_infos[kind];
}

static TokenDelimInfo token_delim_infos[] = {
    [TOKEN_DELIM_PAREN] = { '(', "`(`", ')', "`)`" },
};

TokenDelimInfo token_delim_info(TokenDelim delim) {
    return token_delim_infos[delim];
}

// `TokenStream` consists of an array of `size_t`, interpreted as 'token elements'.
// The first `size_t` is an element contains the 'kind' of the token or token tree.
// Its bit 31 indicates if it's a single token (0) or token group (1). The 31
// lowest bits either contain a `TokenKind` or a `TokenDelim`.
//
// If we have a single token, the next `size_t` contains its starting position in
// the source code. If we have a token with no fixed size (e.g. identifiers,
// numbers), the next size_t contains its end position.
//
// If we have a token group, the next two `size_t` respectively the starting position
// in the source code of its opening and closing delimiter. The next `size_t` contains
// the length of the contents, i.e. the number of `size_t` that the contents take up.

TokenIt token_stream_it(TokenStream stream) {
    size_t const* elements = (size_t const*)stream._elements;
    return (TokenIt) {
        ._cursor = elements,
        ._end = elements + stream._len,
    };
}

int token_it_get(TokenIt it, OUT(TokenTree) dst) {
    return token_it_next(&it, dst);
}

int token_it_next(TokenIt* it, OUT(TokenTree) dst) {
    if (it->_cursor >= it->_end) {
        return -1;
    }
    
    size_t const* cursor = (size_t const*)it->_cursor;
    size_t kind = *(cursor++);

    TokenTree result;

    if (kind >> 31) {
        // token group

        TokenDelim delim = kind & ((1ul << 31) - 1);
        size_t left_start = *(cursor++);
        size_t right_start = *(cursor++);
        size_t len = *(cursor++);
        size_t const* end = cursor + len;

        result = (TokenTree) {
            .kind = TOKEN_TREE_GROUP,
            .as.group = (TokenGroup) {
                .delim = delim,
                .left = (Range) { left_start, left_start + 1 },
                .right = (Range) { right_start, right_start + 1 },
                ._contents = (TokenIt) {
                    ._cursor = cursor,
                    ._end = end,
                }
            }
        };

        cursor = end;
    } else {
        // single token

        TokenKind token_kind = kind & ((1ul << 31) - 1);
        size_t start = *(cursor++);
        TokenKindInfo info = token_kind_info(token_kind);
        size_t end;
        if (info.len == -1) {
            end = *(cursor++);
        } else {
            end = start + info.len;
        }

        result = (TokenTree) {
            .kind = TOKEN_TREE_SINGLE,
            .as.single = (Token) {
                .kind = token_kind,
                .range = (Range) { start, end }
            },
        };
    }

    if (dst) {
        *dst = result;
    }
    it->_cursor = cursor;
    return 0;
}

TokenIt token_group_contents(TokenGroup group) {
    return group._contents;
}

TokenStream token_stream_new(void) {
    return (TokenStream) {
        ._elements = NULL,
        ._len = 0,
        ._capacity = 0,
    };
}

void token_stream_push(TokenStream* stream, Token token) {
    size_t len = stream->_len;
    size_t* elements = stream->_elements;
    reserve(ERASE2(&elements), &stream->_capacity, len + 3, sizeof(size_t));

    elements[len++] = token.kind;
    elements[len++] = token.range.start;
    if (token_kind_info(token.kind).len == -1) {
        elements[len++] = token.range.end;
    }

    stream->_elements = elements;
    stream->_len = len;
}

TokenStreamGroupBuilder token_stream_open_group(
    TokenStream* stream,
    TokenDelim delim,
    size_t start_pos
) {
    size_t* elements = stream->_elements;
    size_t len = stream->_len;
    size_t opening = len;
    reserve(ERASE2(&elements), &stream->_capacity, len + 4, sizeof(size_t));

    elements[len++] = delim | (1ul << 31);
    elements[len++] = start_pos;
    elements[len++] = 0;
    elements[len++] = 0;

    stream->_elements = elements;
    stream->_len = len;

    return (TokenStreamGroupBuilder) {
        ._stream = stream,
        ._opening = opening,
    };
}

void token_stream_close_group(
    OWNED(TokenStreamGroupBuilder) builder,
    size_t start_pos
) {
    size_t* elements = builder->_stream->_elements;
    size_t start = builder->_opening;
    elements[start + 2] = start_pos;
    elements[start + 3] = builder->_stream->_len - (start + 4);
}

void token_stream_free(OWNED(TokenStream) stream) {
    free(stream->_elements);
}
