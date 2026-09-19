#pragma once

#include <stdint.h>

#include "data/ast.h"
#include "data/analysis/id.h"

typedef struct AnFunction {
    AstExpressionId expression;
    uint32_t stack_space;
} AnFunction;

typedef struct AnFunctions {
    BOXED(AnFunction) data;
    size_t _capacity;
    size_t len;
} AnFunctions;

typedef struct AnFunctionIds {
    IdHashTable table; // expression -> `AnFunctionId`
} AnFunctionIds;

AnFunctionId an_functions_register(AnFunctions* nodes, AnFunction v);
