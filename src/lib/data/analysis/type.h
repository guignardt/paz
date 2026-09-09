#pragma once

#include "data/analysis/ptr_hash_table.h"
#include "data/ast.h"
#include "util/alloc.h"

typedef uint32_t AnTypeId;
#define AN_TYPE_ID_INVALID 0
#define AN_TYPE_ID_TYPE 1
#define AN_TYPE_ID_INT 2

typedef struct AnFunctionType {
    AnTypeId input;
    AnTypeId output;
} AnFunctionType;

typedef enum AnTypeKind {
    AN_TYPE_INVALID,
    AN_TYPE_TYPE,
    AN_TYPE_INT,
    AN_TYPE_FUNCTION,
} AnTypeKind;

typedef struct AnType {
    AnTypeKind kind;
    union {
        AnFunctionType function;
    } as;
    uint32_t size;
} AnType;

typedef struct AnTypes {
    BOXED(AnType) _data;
    size_t _len;
    size_t _capacity;
} AnTypes;

AnTypes an_types_new(void);
AnTypes an_types_default(void);
void an_types_free(OWNED(AnTypes) types);

// `id` must be valid
AnType an_types_get(AnTypes types, AnTypeId id);
AnTypeId an_types_get_id_or_insert(AnTypes* types, AnType type);

typedef struct AnExpressions {
    AstExpression const** data;
    size_t len;
    size_t _capacity;
} AnExpressions;

void an_register_expression(AnExpressions* expressions, AstExpression const* v);

typedef struct AnExpressionTypes {
    PtrHashTable table; // values are `AnTypeId`
} AnExpressionTypes;
