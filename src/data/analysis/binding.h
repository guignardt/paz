#pragma once

#include <stdbool.h>

#include "data/analysis/id.h"
#include "data/analysis/ptr_hash_table.h"

#include "util/alloc.h"
#include "util/string.h"

typedef enum AnConstantKind {
    AN_CONSTANT_TYPE,
    AN_CONSTANT_FUNCTION,
} AnConstantKind;

typedef struct AnConstant {
    AnConstantKind kind;
    union {
        AnTypeId type;
        AnFunctionId function;
    } as;
} AnConstant;

typedef struct AnVariable {
    Range registers; // range of `IrRegisterId`s
} AnVariable;

typedef enum AnBindingKind {
    AN_BINDING_CONSTANT,
    AN_BINDING_VARIABLE,
} AnBindingKind;

typedef struct AnBinding {
    String name;
    union {
        AnConstant constant;
        AnVariable variable;
    } as;
} AnBinding;

typedef struct AnScope {
    AnScopeId parent;
    AnBinding* bindings;
    size_t _bindings_capacity;
    size_t bindings_len;
    bool function;
} AnScope;

typedef struct AnScopeBindings {
    AnScope* scopes;
    size_t _scopes_capacity;
    size_t scopes_len;
} AnScopeBindings;

AnScopeId an_new_scope(AnScopeBindings* bindings, AnScopeId parent, bool function);
AnBindingId an_push_binding(AnScopeBindings* bindings, AnScopeId scope, AnBinding binding);

typedef struct AnExpressionScopes {
    PtrHashTable table; // expression -> `AnScopeId`
} AnExpressionScopes;
