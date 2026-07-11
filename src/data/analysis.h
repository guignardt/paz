#pragma once

#include "data/ast.h"
#include "data/analysis/ptr_hash_table.h"
#include "data/analysis/type.h"
#include "data/analysis/function.h"
#include "data/analysis/binding.h"

typedef struct AnExpressionTypes {
    PtrHashTable table; // values are `AnTypeId`
} AnExpressionTypes;
