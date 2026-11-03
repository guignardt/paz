#pragma once

#include "data/analysis/id.h"
#include "data/analysis/id_hash_table.h"
#include "data/ast.h"
#include "util/alloc.h"

#define AN_TYPE_ID_INVALID 0
#define AN_TYPE_ID_NEVER 1
#define AN_TYPE_ID_TYPE 2
#define AN_TYPE_ID_INT 3
#define AN_TYPE_UNIT 4

typedef struct AnFunctionType {
    AnTypeId input;
    AnTypeId output;
} AnFunctionType;

typedef struct AnTupleType {
    BOXED(AnTypeId) elements;
    size_t len;
} AnTupleType;

typedef enum AnTypeKind {
    AN_TYPE_INVALID,
    AN_TYPE_NEVER,
    AN_TYPE_TYPE,
    AN_TYPE_INT,
    AN_TYPE_FUNCTION,
    AN_TYPE_TUPLE,
} AnTypeKind;

typedef struct AnType {
    AnTypeKind kind;
    union {
        AnFunctionType function;
        AnTupleType tuple;
    } as;
    uint32_t size;
} AnType;

typedef struct AnTypes {
    DYNAMIC_ARRAY(AnType) _types;
} AnTypes;

AnTypes an_types_new(void);
AnTypes an_types_default(void);
void an_types_free(OWNED(AnTypes) types);

// `id` must be valid
AnType an_types_get(AnTypes types, AnTypeId id);
// frees elements of tuple if not inserted
// FIXME: this is inconsistent, type should take a pointer
AnTypeId an_types_get_id_or_insert(AnTypes* types, AnType type);

typedef AnTypeId* AnExpressionTypes;
typedef AnTypeId* AnPatternTypes;
