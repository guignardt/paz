#pragma once

#include <stdint.h>

#include "data/ast.h"
#include "data/analysis/id.h"

typedef struct AnFunction {
    AstExpression const* expression;
    AnBindingId* captured_bindings;
    size_t captured_bindings_len;
} AnFunction;

typedef struct AnFunctions {
    BOXED(AnFunction) functions;
    size_t _functions_capacity;
    size_t functions_len;
} AnFunctions;

AnFunctionId an_functions_register(AnFunctions* nodes, AnFunction v);
