#pragma once

#include "data/ast.h"
#include "data/analysis.h"

typedef struct AnalyzeWalk {
    AnFunctions* functions;
    AnFunctionIds function_ids;
    AnScopeBindings* scope_bindings;
    AnExpressionScopes* expression_scopes;
    AnBindingIds* binding_ids;
    AnConstantBindings* constant_bindings;
    AnScopeId _scope;
    AnBindingKind _binding_kind;
} AnalyzeWalk;

void analyze_walk_module(AnalyzeWalk w, AstModule const* v);
void analyze_walk_expression(AnalyzeWalk w, AstExpression const* v);
void analyze_walk_type_name(AnalyzeWalk w, AstTypeName const* v);
void analyze_walk_pattern(AnalyzeWalk w, AstPattern const* v);
void analyze_walk_binding(AnalyzeWalk w, AstBinding const* v);

typedef struct AnalyzeConstantBindings {
    AnFunctions functions;
    AnFunctionIds function_ids;
    AnScopeBindings* scope_bindings;
    AnBindingIds binding_ids;
    AnConstantBindings constant_bindings;
} AnalyzeConstantBindings;

void analyze_constant_bindings(AnalyzeConstantBindings a);
