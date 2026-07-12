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

void an_register_constant_binding(AnConstantBindings* bindings, AstBinding const* v) {
    reserve(ERASE2(&bindings->data), &bindings->_capacity, bindings->len + 1, sizeof(v));
    bindings->data[bindings->len++] = v;
}
