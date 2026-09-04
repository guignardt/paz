#pragma once

#include <stdbool.h>

#include "data/ast.h"
#include "data/analysis/id.h"
#include "data/analysis/constant.h"
#include "data/analysis/ptr_hash_table.h"

#include "util/alloc.h"
#include "util/string.h"

typedef struct AnVariable {
    Range registers; // range of `IrRegisterId`s
} AnVariable;

typedef enum AnBindingKind {
    AN_BINDING_CONSTANT,
    AN_BINDING_VARIABLE,
} AnBindingKind;

typedef struct AnBinding {
    String name;
    AnBindingKind kind;
    union {
        AnConstant constant;
        AnVariable variable;
    } as;
    AstBinding const* source_binding;
    AnTypeId type;
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
AnBinding const* an_get_binding(AnScopeBindings bindings, AnBindingId id);
AnBinding* an_get_binding_mut(AnScopeBindings* bindings, AnBindingId id);

int an_find_binding_in_scope(AnScopeBindings bindings, AnScopeId scope, String s, OUT(AnBindingId) id);
int an_find_binding(AnScopeBindings bindings, AnScopeId scope, String s, OUT(AnBindingId) id);

typedef struct AnReferenceScopes {
    PtrHashTable table; // `AstExpression*` (of kind reference) -> `AnScopeId`
} AnReferenceScopes;

typedef struct AnBindingIds {
    // `AstPattern*` (of kind variable)
    // or `AstExpression` (of kind reference)
    // -> `AnBindingId`
    PtrHashTable table;
} AnBindingIds;

typedef struct AnConstantBindings {
    BOXED(AstBinding const*) data;
    size_t len;
    size_t _capacity;
} AnConstantBindings;

void an_register_constant_binding(AnConstantBindings* bindings, AstBinding const* v);
