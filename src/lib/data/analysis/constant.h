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

typedef IdHashTable /* expression -> `AnConstant` */ AnConstants;

typedef enum AnConstantItemKind {
    AN_CONSTANT_ITEM_EXPRESSION_TYPE,
    AN_CONSTANT_ITEM_EXPRESSION,
    AN_CONSTANT_ITEM_PATTERN,
    AN_CONSTANT_ITEM_BINDING,
} AnConstantItemKind;

typedef struct AnConstantItem {
    unsigned int kind : 2; // `AnConstantItemKind`
    Id id : 30; // `AstExpressionId` or `AstBindingSetId`
} AnConstantItem;

typedef struct AnConstantItems {
    DYNAMIC_ARRAY(AstExpressionId) expression_types;
    DYNAMIC_ARRAY(AstExpressionId) expressions;
    DYNAMIC_ARRAY(AstExpressionId) patterns;
    DYNAMIC_ARRAY(AstExpressionId) binding_sets;
} AnConstantItems;
