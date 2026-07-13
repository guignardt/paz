#pragma once

#include "data/ast.h"
#include "data/analysis.h"

// 1. walk the AST, building the scope tree, registering constant bindings,
// registering functions.

// 2a. evaluate the values of constant bindings and compute their types.
// 2b. match the value of all constant bindings to their pattern, type
// the constant bindings.

// 3. walk the AST, registering let bindings and typing non-constant values

typedef struct AnalyzeWalk {
    Analysis* analysis;
    AnScopeId _scope;
    AnBindingKind _binding_kind;
} AnalyzeWalk;

AnalyzeWalk analyze_walk_init(Analysis* analysis);

void analyze_walk_const_module(AnalyzeWalk w, AstModule const* v);
void analyze_walk_const_expression(AnalyzeWalk w, AstExpression const* v);
void analyze_walk_const_type_name(AnalyzeWalk w, AstTypeName const* v);
void analyze_walk_const_pattern(AnalyzeWalk w, AstPattern const* v);
void analyze_walk_const_binding(AnalyzeWalk w, AstBinding const* v);

void analyze_evaluate_const_bindings(Analysis* a);

// void analyze_constant_bindings(Analysis* a);
