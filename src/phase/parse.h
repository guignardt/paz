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
    AstStorage* storage;
} Parser;

// valid tokens must be passed to parsing functions: for example, the text of
// a `TOKEN_INT` must be of the correct form (an optional sign followed by
// decimal digits).

ParseStatus parse_program(Parser parser, OUT(AstProgram*) dst);
ParseStatus parse_module(Parser parser, OUT(AstModule*) dst);
ParseStatus parse_binding(Parser parser, OUT(AstBinding*) dst);
ParseStatus parse_pattern(Parser parser, OUT(AstPattern*) dst);
ParseStatus parse_expression(Parser parser, OUT(AstExpression*) dst);
ParseStatus parse_type_name(Parser parser, OUT(AstTypeName*) dst);
