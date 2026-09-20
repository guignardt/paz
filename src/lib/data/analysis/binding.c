#include <string.h>

#include "data/analysis.h"

typedef struct AnScope {
    AnScopeLocation parent;
    DYNAMIC_ARRAY(AnBinding) constant_bindings;
    DYNAMIC_ARRAY(AnBinding) variable_bindings;
    bool function;
} AnScope;

AnScopeBindings an_scope_bindings_new(void) {
    return (AnScopeBindings) { ._scopes = {0} };
}

AnScopeId an_new_scope(AnScopeBindings* bindings, AnScopeLocation parent, bool function) {
    AnScopeId id = bindings->_scopes.len;
    AnScope scope = {
        .parent = parent,
        .constant_bindings = {0},
        .variable_bindings = {0},
        .function = function,
    };
    DYNAMIC_ARRAY_PUSH(&bindings->_scopes, scope);
    return id;
}

AnBindingId an_add_binding(AnScopeBindings* bindings, AnScopeId scope_id, AnBinding binding) {
    AnScope* scope = &bindings->_scopes.data[scope_id];
    size_t index;
    switch (binding.kind) {
    case AN_BINDING_CONSTANT:
        index = scope->constant_bindings.len;
        DYNAMIC_ARRAY_PUSH(&scope->constant_bindings, binding);
        return (AnBindingId) { .scope = scope_id, ._constant = true, ._index = index };
    case AN_BINDING_VARIABLE:
        index = scope->variable_bindings.len;
        DYNAMIC_ARRAY_PUSH(&scope->variable_bindings, binding);
        return (AnBindingId) { .scope = scope_id, ._constant = false, ._index = index };
    }
}

AnBinding const* an_get_binding(AnScopeBindings bindings, AnBindingId id) {
    if (id._constant) {
        return &bindings._scopes.data[id.scope].constant_bindings.data[id._index];
    } else {
        return &bindings._scopes.data[id.scope].constant_bindings.data[id._index];
    }
}

AnBinding* an_get_binding_mut(AnScopeBindings* bindings, AnBindingId id) {
        if (id._constant) {
        return &bindings->_scopes.data[id.scope].constant_bindings.data[id._index];
    } else {
        return &bindings->_scopes.data[id.scope].constant_bindings.data[id._index];
    }
}

int an_find_binding_in_scope(AnScopeBindings bindings, AnScopeLocation loc, String s, OUT(AnBindingId) id) {
    AnScope const* scope = &bindings._scopes.data[loc.scope];

    for (intptr_t i = (intptr_t)loc._location - 1; i >= 0; i--) {
        AnBinding b = scope->variable_bindings.data[i];
        if (b.name.len == s.len && !strncmp(b.name.data, s.data, s.len)) {
            if (id) *id = (AnBindingId) { .scope = loc.scope, ._constant = false, ._index = i };
            return 0;
        }
    }

    for (intptr_t i = 0; i < scope->constant_bindings.len; i++) {
        AnBinding b = scope->constant_bindings.data[i];
        if (b.name.len == s.len && !strncmp(b.name.data, s.data, s.len)) {
            if (id) *id = (AnBindingId) { .scope = loc.scope, ._constant = true, ._index = i };
            return 0;
        }
    }

    return -1;
}

int an_find_binding(AnScopeBindings bindings, AnScopeLocation loc, String s, OUT(AnBindingId) id) {
    while (loc.scope != ID_SENTINEL) {
        if (!an_find_binding_in_scope(bindings, loc, s, id)) {
            return 0;
        }
        loc = bindings._scopes.data[loc.scope].parent;
    }
    return -1;
}

AnScopeLocation an_scope_end_location(AnScopeBindings bindings, AnScopeId scope) {
    return (AnScopeLocation) {
        .scope = scope,
        ._location = bindings._scopes.data[scope].variable_bindings.len,
    };
}

AnScopeLocation an_scope_constant_location(AnScopeId scope) {
    return (AnScopeLocation) {
        .scope = scope,
        ._location = 0,
    };
}
