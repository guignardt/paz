#pragma once

#include <stdint.h>

typedef Id AnTypeId;

typedef Id AnFunctionId;

typedef Id AnScopeId;

typedef struct AnBindingId {
    AnScopeId scope;
    bool _constant : 1;
    uint32_t _index : 31;
} AnBindingId;
