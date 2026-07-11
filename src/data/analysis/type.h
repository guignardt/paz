#pragma once

#include "util/alloc.h"

typedef uint32_t AnTypeId;
#define AN_TYPE_ID_INVALID 0
#define AN_TYPE_ID_INT 1

typedef struct AnFunctionType {
    AnTypeId input;
    AnTypeId output;
} AnFunctionType;

typedef enum AnTypeKind {
    AN_TYPE_INVALID,
    AN_TYPE_INT,
    AN_TYPE_FUNCTION,
    AN_TYPE_FLOW,
} AnTypeKind;

typedef struct AnType {
    AnTypeKind kind;
    union {
        AnFunctionType function;
        AnTypeId flow;
    } as;
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
