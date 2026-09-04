#pragma once

#include <stdint.h>

#include "data/ast.h"
#include "data/analysis/id.h"

typedef struct AnFunction {
    AstExpression const* expression;
} AnFunction;

typedef struct AnFunctions {
    BOXED(AnFunction) data;
    size_t _capacity;
    size_t len;
} AnFunctions;

typedef struct AnFunctionIds {
    PtrHashTable table; // `AstExpression*` (of kind function) -> `AnFunctionId`
} AnFunctionIds;

AnFunctionId an_functions_register(AnFunctions* nodes, AnFunction v);
