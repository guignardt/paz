#pragma once

#include <stdint.h>

typedef uint32_t AnTypeId;

typedef uint32_t AnFunctionId;

typedef uint32_t AnScopeId;
#define AN_SCOPE_ID_SENTINEL ((AnScopeId)(-1))

typedef struct AnBindingId {
    AnScopeId scope;
    uint32_t _index;
} AnBindingId;

