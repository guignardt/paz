#pragma once

#include "data/ast.h"
#include "data/analysis.h"

void analyze_prepare(Analysis* a, bool consteval);
void analyze_resolve_bindings(Analysis* a);
void analyze_evaluate(Analysis* a);
