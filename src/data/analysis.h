#pragma once

#include "data/ast.h"
#include "data/analysis/ptr_hash_table.h"
#include "data/analysis/type.h"
#include "data/analysis/function.h"
#include "data/analysis/binding.h"

typedef struct Analysis {
    AnFunctions functions;
    AnFunctionIds function_ids;
    AnScopeBindings scope_bindings;
    AnExpressionScopes expression_scopes;
    AnBindingIds binding_ids;
    AnConstantBindings constant_bindings;
    AnExpressions expressions;
    AnExpressionTypes expression_types;
} Analysis;

Analysis analysis_new(void);
