#pragma once

#include <stdint.h>

#include "data/analysis/function.h"
#include "data/analysis/id_hash_table.h"

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
    IdHashTable table; // expression -> `AnConstant`
} AnConstants;

typedef enum AnConstantItemKind {
    AN_CONSTANT_ITEM_EXPRESSION,
    AN_CONSTANT_ITEM_BINDING,
} AnConstantItemKind;

typedef struct AnConstantItem {
    AnConstantItemKind kind;
    uint32_t id; // `AstExpressionId` or `AstBindingSetId`
} AnConstantItem;

typedef DYNAMIC_ARRAY(AnConstantItem, AnConstantItems) AnConstantItems;
