#pragma once

#include "data/ast.h"
#include "data/analysis.h"

// 1. walk the AST, building the scope tree, registering constant bindings,
// registering functions, registering constant expressions and bindings.

// 2a. evaluate the values of constant bindings and compute their types.
// 2b. match the value of all constant bindings to their pattern, type
// the constant bindings.

// 3. walk the AST, registering let bindings and typing non-constant values

typedef struct Analyzer {
    Analysis* analysis;
    AnScopeId _scope;
    AnBindingKind _binding_kind;
    AstBinding const* _binding;
} Analyzer;

Analyzer analyzer_init(Analysis* analysis);

void analyze_prepare_module(Analyzer w, AstModule const* v);
void analyze_prepare_expression(Analyzer w, AstExpression const* v, bool explicitly_constant);
void analyze_prepare_pattern(Analyzer w, AstPattern const* v);
void analyze_prepare_binding(Analyzer w, AstBinding const* v);

void analyze_const(Analyzer w);
