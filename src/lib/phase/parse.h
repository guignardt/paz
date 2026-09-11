#pragma once

#include "data/token.h"
#include "data/ast.h"

// status of the token iterator after parsing
typedef enum ParseStatus {
    // the iterator is in a correct position.
    PARSE_OK = 0,

    // parsing should not continue directly. Before continuing, the parser should
    // recover in some way.
    PARSE_ILL = -1,
} ParseStatus;

typedef struct Parser {
    char const* text;
    TokenIt* tokens;
    // AstStorage* storage;
    AstNodes* nodes;
} Parser;

// valid tokens must be passed to parsing functions: for example, the text of
// a `TOKEN_INT` must be of the correct form (an optional sign followed by
// decimal digits).

ParseStatus parse_module(Parser parser, OUT(AstModule) dst);
ParseStatus parse_binding_set(Parser parser, OUT(AstBindingSetId) dst);
ParseStatus parse_pattern(Parser parser, OUT(AstPatternId) dst);
ParseStatus parse_expression(Parser parser, OUT(AstExpressionId) dst);
