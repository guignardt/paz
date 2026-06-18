#pragma once

#include "data/token.h"
#include "data/ast.h"

typedef enum ParseStatus {
    // everything went fine
    PARSE_OK = 0,

    // the result is a valid object but an error occurred which put the Token
    // iterator in a position where parsing possibly can't continue directly.
    PARSE_ILL = -1,

    // no result was written. An error occurred which put the Token
    // iterator in a position where parsing possibly can't continue directly. 
    PARSE_ERROR = -2,
} ParseStatus;

typedef struct Parser {
    char const* text;
    TokenIt* tokens;
    AstStorage* storage;
} Parser;

// valid tokens must be passed to parsing functions: for example, the text of
// a `TOKEN_INT` must be of the correct form (an optional sign followed by
// decimal digits).

ParseStatus parse_program(Parser parser, OUT(AstProgram) dst);
ParseStatus parse_module(Parser parser, OUT(AstModule) dst);
ParseStatus parse_binding(Parser parser, OUT(AstBinding) dst);
ParseStatus parse_pattern(Parser parser, OUT(AstPattern) dst);
ParseStatus parse_expression(Parser parser, OUT(AstExpression) dst);
ParseStatus parse_type_name(Parser parser, OUT(AstTypeName) dst);

bool parse_status_returned(ParseStatus status);
bool parse_status_ill(ParseStatus status);
