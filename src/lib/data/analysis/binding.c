#include <string.h>

#include "data/analysis.h"

typedef struct AnScope {
    AnScopeLocation parent;

    AnBinding* constant_bindings;
    size_t constant_bindings_cap;
    size_t constant_bindings_len;

    AnBinding* variable_bindings;
    size_t variable_bindings_cap;
    size_t variable_bindings_len;

    bool function;
} AnScope;

AnScopeBindings an_scope_bindings_new(void) {
    return (AnScopeBindings) {
        ._scopes = NULL,
        ._scopes_capacity = 0,
        .scopes_len = 0,
    };
}

AnScopeId an_new_scope(AnScopeBindings* bindings, AnScopeLocation parent, bool function) {
    AnScopeId id = bindings->scopes_len;
    reserve(ERASE2(&bindings->_scopes), &bindings->_scopes_capacity, bindings->scopes_len + 1, sizeof(AnScope));
    bindings->_scopes[bindings->scopes_len++] = (AnScope) {
        .parent = parent,

        .constant_bindings = NULL,
        .constant_bindings_cap = 0,
        .constant_bindings_len = 0,

        .variable_bindings = NULL,
        .variable_bindings_cap = 0,
        .variable_bindings_len = 0,

        .function = function,
    };
    return id;
}

AnBindingId an_add_binding(AnScopeBindings* bindings, AnScopeId scope_id, AnBinding binding) {
    AnScope* scope = &bindings->_scopes[scope_id];
    size_t index;
    switch (binding.kind) {
    case AN_BINDING_CONSTANT:
        index = scope->constant_bindings_len;
        reserve(
            ERASE2(&scope->constant_bindings),
            &scope->constant_bindings_cap,
            scope->constant_bindings_len + 1,
            sizeof(AnBinding)
        );
        scope->constant_bindings[scope->constant_bindings_len++] = binding;
        return (AnBindingId) { .scope = scope_id, ._constant = true, ._index = index };
    case AN_BINDING_VARIABLE:
        index = scope->variable_bindings_len;
        reserve(
            ERASE2(&scope->variable_bindings),
            &scope->variable_bindings_cap,
            scope->variable_bindings_len + 1,
            sizeof(AnBinding)
        );
        scope->variable_bindings[scope->variable_bindings_len++] = binding;
        return (AnBindingId) { .scope = scope_id, ._constant = false, ._index = index };
    }
}

void an_register_constant_binding(AnConstantBindings* bindings, AstBindingSet const* v) {
    reserve(ERASE2(&bindings->data), &bindings->_capacity, bindings->len + 1, sizeof(v));
    bindings->data[bindings->len++] = v;
}

AnBinding const* an_get_binding(AnScopeBindings bindings, AnBindingId id) {
    if (id._constant) {
        return &bindings._scopes[id.scope].constant_bindings[id._index];
    } else {
        return &bindings._scopes[id.scope].constant_bindings[id._index];
    }
}

AnBinding* an_get_binding_mut(AnScopeBindings* bindings, AnBindingId id) {
        if (id._constant) {
        return &bindings->_scopes[id.scope].constant_bindings[id._index];
    } else {
        return &bindings->_scopes[id.scope].constant_bindings[id._index];
    }
}

int an_find_binding_in_scope(AnScopeBindings bindings, AnScopeLocation loc, String s, OUT(AnBindingId) id) {
    AnScope const* scope = &bindings._scopes[loc.scope];

    for (intptr_t i = loc._location - 1; i >= 0; i--) {
        AnBinding b = scope->variable_bindings[i];
        if (b.name.len == s.len && !strncmp(b.name.data, s.data, s.len)) {
            if (id) *id = (AnBindingId) { .scope = loc.scope, ._constant = false, ._index = i };
            return 0;
        }
    }

    for (intptr_t i = 0; i < scope->constant_bindings_len; i++) {
        AnBinding b = scope->constant_bindings[i];
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
        loc = bindings._scopes[loc.scope].parent;
    }
    return -1;
}
