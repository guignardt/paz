#pragma once

#include "data/ast.h"
#include "data/analysis/type.h"
#include "data/analysis/function.h"
#include "data/analysis/constant.h"
#include "data/analysis/binding.h"

typedef struct Analysis {
    AstNodeView root;
    AnFunctions functions;
    AnFunctionIds function_ids;
    AnScopeBindings scope_bindings;
    AnReferenceBindingLocations reference_binding_locations;
    AnVariableBindings variable_bindings;
    AnPatternBindingSets pattern_binding_sets;
    AnConstants constants;
    AnConstantItems constant_items;
    AnTypes types;
    AnExpressionTypes expression_types;
    AnPatternTypes pattern_types;
} Analysis;

Analysis analysis_new(AstNodeView root);
