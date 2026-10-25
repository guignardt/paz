#pragma once

#include "data/ast.h"
#include "data/analysis.h"
#include "data/bytecode.h"

Bytecode compile(AstNodes ast, Analysis a);
