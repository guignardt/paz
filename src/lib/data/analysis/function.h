#pragma once

#include <stdint.h>

#include "data/ast.h"
#include "data/analysis/id.h"
#include "data/analysis/id_hash_table.h"

typedef struct AnFunction {
    AstExpressionId expression;
    uint32_t stack_space;
} AnFunction;

typedef DYNAMIC_ARRAY(AnFunction, AnFunctions) AnFunctions;

typedef struct AnFunctionIds {
    IdHashTable table; // expression -> `AnFunctionId`
} AnFunctionIds;

AnFunctionId an_functions_register(AnFunctions* functions, AnFunction v);
