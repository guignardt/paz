#pragma once

#include "phase/parse.h"

AstExpressionId box_expression(Parser p, AstExpression v);
AstPatternId box_pattern(Parser p, AstPattern v);
AstBindingSetId box_binding_set(Parser p, AstBindingSet v);

void unexpected_token(void);
