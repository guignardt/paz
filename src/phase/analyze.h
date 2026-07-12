#pragma once

#include "data/ast.h"
#include "data/analysis.h"

typedef struct AnalyzeWalk {
    Analysis* analysis;
    AnScopeId _scope;
    AnBindingKind _binding_kind;
} AnalyzeWalk;

AnalyzeWalk analyze_walk_init(Analysis* analysis);

void analyze_walk_module(AnalyzeWalk w, AstModule const* v);
void analyze_walk_expression(AnalyzeWalk w, AstExpression const* v);
void analyze_walk_type_name(AnalyzeWalk w, AstTypeName const* v);
void analyze_walk_pattern(AnalyzeWalk w, AstPattern const* v);
void analyze_walk_binding(AnalyzeWalk w, AstBinding const* v);

void analyze_constant_bindings(Analysis* a);
