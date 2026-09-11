#include <string.h>

#include "data/analysis.h"

AnScopeId an_new_scope(AnScopeBindings* bindings, AnScopeId parent, bool function) {
    AnScopeId id = bindings->scopes_len;
    reserve(ERASE2(&bindings->scopes), &bindings->_scopes_capacity, bindings->scopes_len + 1, sizeof(AnScope));
    bindings->scopes[bindings->scopes_len++] = (AnScope) {
        .parent = parent,
        .bindings = NULL,
        .bindings_len = 0,
        ._bindings_capacity = 0,
        .function = function,
    };
    return id;
}

AnBindingId an_push_binding(AnScopeBindings* bindings, AnScopeId scope_id, AnBinding binding) {
    AnScope* scope = &bindings->scopes[scope_id];
    size_t index = scope->bindings_len;
    reserve(ERASE2(&scope->bindings), &scope->_bindings_capacity, scope->bindings_len + 1, sizeof(AnBinding));
    scope->bindings[scope->bindings_len++] = binding;
    return (AnBindingId) { .scope = scope_id, ._index = index };
}

void an_register_constant_binding(AnConstantBindings* bindings, AstBindingSet const* v) {
    reserve(ERASE2(&bindings->data), &bindings->_capacity, bindings->len + 1, sizeof(v));
    bindings->data[bindings->len++] = v;
}

AnBinding const* an_get_binding(AnScopeBindings bindings, AnBindingId id) {
    return &bindings.scopes[id.scope].bindings[id._index];
}

AnBinding* an_get_binding_mut(AnScopeBindings* bindings, AnBindingId id) {
    return &bindings->scopes[id.scope].bindings[id._index];
}

int an_find_binding_in_scope(AnScopeBindings bindings, AnScopeId scope_id, String s, OUT(AnBindingId) id) {
    AnScope const* scope = &bindings.scopes[scope_id];
    for (intptr_t i = (intptr_t)scope->bindings_len - (intptr_t)1; i >= 0; i--) {
        AnBinding const* b = &scope->bindings[i];
        if (b->name.len == s.len && !strncmp(b->name.data, s.data, s.len)) {
            if (id) *id = (AnBindingId) { .scope = scope_id, ._index = i };
            return 0;
        }
    }
    return -1;
}

int an_find_binding(AnScopeBindings bindings, AnScopeId scope_id, String s, OUT(AnBindingId) id) {
    while (scope_id != AN_SCOPE_ID_SENTINEL) {
        if (!an_find_binding_in_scope(bindings, scope_id, s, id)) {
            return 0;
        }
        scope_id = bindings.scopes[scope_id].parent;
    }
    return -1;
}
