#pragma once

#include <stdint.h>

#include "data/analysis/function.h"
#include "data/analysis/ptr_hash_table.h"

typedef enum AnConstantKind {
    AN_CONSTANT_INVALID,
    AN_CONSTANT_TYPE,
    AN_CONSTANT_FUNCTION,
} AnConstantKind;

typedef struct AnConstant {
    AnConstantKind kind;
    union {
        AnTypeId type;
        AnFunctionId function;
    } as;
    AnTypeId type;
} AnConstant;

typedef struct AnConstants {
    PtrHashTable table; // `AstExpression*` -> `AnConstant`
} AnConstants;

typedef enum AnConstantItemKind {
    AN_CONSTANT_ITEM_EXPRESSION,
    AN_CONSTANT_ITEM_BINDING,
} AnConstantItemKind;

typedef struct AnConstantItem {
    AnConstantItemKind kind;
    union {
        AstExpression const* expression;
        AstBinding const* binding;
    } as;
} AnConstantItem;

typedef struct AnConstantItems {
    OWNED(AnConstantItem) data;
    size_t len;
    size_t _capacity;
} AnConstantItems;
