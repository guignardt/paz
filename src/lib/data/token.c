#include <stdbool.h>
#include <stdint.h>

#include "data/token.h"

#include "util/debug.h"

static TokenKindInfo token_kind_infos[] = {
    [TOKEN_COMMA] =             {   1,  ",",        "`,`"           },
    [TOKEN_COLON] =             {   1,  ":",        "`:`"           },
    [TOKEN_SEMICOLON] =         {   1,  ";",        "`;`"           },
    [TOKEN_COLON_EQUAL] =       {   2,  ":=",       "`:=`"          },
    [TOKEN_COLON_COLON] =       {   2,  "::",       "`::`"          },
    [TOKEN_EQUAL] =             {   1,  "=",        "`=`"           },
    [TOKEN_ARROW] =             {   2,  "->",       "`->`"          },
    [TOKEN_DOUBLE_ARROW] =      {   2,  "=>",       "`=>`"          },

    [TOKEN_EQUAL_EQUAL] =       {   2,  "==",       "`==`"          },
    [TOKEN_BANG_EQUAL] =        {   2,  "!=",       "`!=`"          },
    [TOKEN_LESS] =              {   1,  "<",        "`<`"           },
    [TOKEN_LESS_EQUAL] =        {   2,  "<=",       "`<=`"          },
    [TOKEN_GREATER] =           {   1,  ">",        "`>`"           },
    [TOKEN_GREATER_EQUAL] =     {   2,  ">=",       "`>=`"          },

    [TOKEN_PLUS] =              {   1,  "+",        "`+`"           },
    [TOKEN_MINUS] =             {   1,  "-",        "`-`"           },
    [TOKEN_STAR] =              {   1,  "*",        "`*`"           },
    [TOKEN_SLASH] =             {   1,  "/",        "`/`"           },
    [TOKEN_PERCENT] =           {   1,  "%",        "`%`"           },

    [TOKEN_CONST] =             {   5,  "const",    "`const`"       },
    [TOKEN_LET] =               {   3,  "let",      "`let`"         },
    [TOKEN_FN] =                {   2,  "fn",       "`fn`"          },
    [TOKEN_FOR] =               {   3,  "for",      "`for`"         },
    [TOKEN_BREAK] =             {   5,  "break",    "`break`"       },
    [TOKEN_CONTINUE] =          {   8,  "continue", "`continue`"    },
    [TOKEN_IF] =                {   2,  "if",       "`if`"          },
    [TOKEN_THEN] =              {   4,  "then",     "`then`"        },
    [TOKEN_ELSE] =              {   4,  "else",     "`else`"        },

    [TOKEN_INT] =               {   -1, {0},        "<integer>"     },

    [TOKEN_IDENTIFIER] =        {   -1, {0},        "<identifier>"  },
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

// `TokenStream` consists of an array of `uint32_t`, interpreted as 'token elements'.
// The first `uint32_t` is an element contains the 'kind' of the token or token tree.
// Its bit 31 indicates if it's a single token (0) or token group (1). The 31
// lowest bits either contain a `TokenKind` or a `TokenDelim`.
//
// If we have a single token, the next `uint32_t` contains its starting position in
// the source code. If we have a token with no fixed size (e.g. identifiers,
// numbers), the next uint32_t contains its end position.
//
// If we have a token group, the next two `uint32_t` respectively the starting position
// in the source code of its opening and closing delimiter. The next `uint32_t` contains
// the length of the contents, i.e. the number of `uint32_t` that the contents take up.

TokenIt token_stream_it(TokenStream stream) {
    uint32_t const* elements = (uint32_t const*)stream._elements;
    return (TokenIt) {
        ._cursor = elements,
        ._end = elements + stream._len,
    };
}

bool token_it_end(TokenIt it) {
    return it._cursor >= it._end;
}

int token_it_get(TokenIt it, OUT(TokenTree) dst) {
    return token_it_next(&it, dst);
}

int token_it_next(TokenIt* it, OUT(TokenTree) dst) {
    if (token_it_end(*it)) {
        return -1;
    }
    
    uint32_t const* cursor = (uint32_t const*)it->_cursor;
    uint32_t kind = *(cursor++);

    TokenTree result;

    if (kind >> 31) {
        // token group

        TokenDelim delim = kind & ((1ul << 31) - 1);
        uint32_t left_start = *(cursor++);
        uint32_t right_start = *(cursor++);
        uint32_t len = *(cursor++);
        uint32_t const* end = cursor + len;

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
        uint32_t start = *(cursor++);
        TokenKindInfo info = token_kind_info(token_kind);
        uint32_t end;
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

int token_it_match_single(TokenIt* it, TokenKind kind, OUT(Token) dst) {
    TokenIt it1 = *it;
    TokenTree tree = {0};
    if (
        token_it_next(&it1, &tree)
        || tree.kind != TOKEN_TREE_SINGLE
        || tree.as.single.kind != kind
    ) {
        return -1;
    }
    *it = it1;
    if (dst) {
        *dst = tree.as.single;
    }
    return 0;
}

int token_it_match_group(TokenIt* it, TokenDelim delim, OUT(TokenGroup) dst) {
    TokenIt it1 = *it;
    TokenTree tree = {0};
    if (
        token_it_next(&it1, &tree)
        || tree.kind != TOKEN_TREE_GROUP
        || tree.as.group.delim != delim
    ) {
        return -1;
    }
    *it = it1;
    if (dst) {
        *dst = tree.as.group;
    }
    return 0;
}

int token_it_find_single(TokenIt* it, TokenKind kind, OUT(Token) dst) {
    while (true) {
        TokenTree tree;
        if (token_it_next(it, &tree)) {
            return -1;
        }
        if (tree.kind != TOKEN_TREE_SINGLE || tree.as.single.kind != kind) {
            continue;
        }
        if (dst) {
            *dst = tree.as.single;
        }
        return 0;
    }
}

int token_it_find_group(TokenIt* it, TokenDelim delim, OUT(TokenGroup) dst) {
    while (true) {
        TokenTree tree;
        if (token_it_next(it, &tree)) {
            return -1;
        }
        if (tree.kind != TOKEN_TREE_GROUP || tree.as.group.delim == delim) {
            continue;
        }
        if (dst) {
            *dst = tree.as.group;
        }
        return 0;
    }
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
    uint32_t len = stream->_len;
    uint32_t* elements = stream->_elements;
    reserve(ERASE2(&elements), &stream->_capacity, len + 3, sizeof(uint32_t));

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
    uint32_t* elements = stream->_elements;
    uint32_t len = stream->_len;
    uint32_t opening = len;
    reserve(ERASE2(&elements), &stream->_capacity, len + 4, sizeof(uint32_t));

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
    uint32_t* elements = builder->_stream->_elements;
    uint32_t start = builder->_opening;
    elements[start + 2] = start_pos;
    elements[start + 3] = builder->_stream->_len - (start + 4);
}

void token_stream_free(OWNED(TokenStream) stream) {
    free(stream->_elements);
}

void debug_tokens(TokenIt it) {
    TokenTree token_tree;
    while (!token_it_next(&it, &token_tree)) {
        switch (token_tree.kind) {
            case TOKEN_TREE_SINGLE:
                debug_begin("token");
                debug_attr_int("kind", token_tree.as.single.kind);
                debug_attr_range("range", token_tree.as.single.range);
                debug_end();
                break;
            case TOKEN_TREE_GROUP:
                debug_begin("group");
                debug_attr_int("delim", token_tree.as.group.delim);
                debug_attr_range("range", (Range) {
                    token_tree.as.group.left.start,
                    token_tree.as.group.right.end,
                });
                debug_attr_begin_list("contents");
                debug_tokens(token_group_contents(token_tree.as.group));
                debug_attr_end_list();
                debug_end();
                break;
        }
    }
}
