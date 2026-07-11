#pragma once

#include "data/ast.h"
#include "data/analysis.h"

typedef struct AnalyzeWalker {
    AnFunctions* functions;
    AnScopeBindings* bindings;
    AnExpressionScopes* expression_scopes;
    AnScopeId _scope;
} AnalyzeWalker;

void analyze_walk_module(AnalyzeWalker w, AstModule const* v);
void analyze_walk_expression(AnalyzeWalker w, AstExpression const* v);
void analyze_walk_type_name(AnalyzeWalker w, AstTypeName const* v);
void analyze_walk_pattern(AnalyzeWalker w, AstPattern const* v);
void analyze_walk_binding(AnalyzeWalker w, AstBinding const* v);

// evaluates constant bindings
void analyze_evaluate_constants();

void analyze_types();
