#pragma once

#include <stdbool.h>

#include "data/ast.h"
#include "data/analysis/id.h"
#include "data/analysis/constant.h"
#include "data/analysis/id_hash_table.h"

#include "util/alloc.h"
#include "util/string.h"

typedef struct AnVariable {
    Range registers;
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
    AstPatternId top_level_pattern;
    AnTypeId type;
} AnBinding;

typedef struct AnScopeLocation {
    AnScopeId scope;
    uint32_t _location;
} AnScopeLocation;

typedef struct AnScopeBindings {
    DYNAMIC_ARRAY(struct AnScope) _scopes;
} AnScopeBindings;

AnScopeBindings an_scope_bindings_new(void);

AnScopeId an_new_scope(AnScopeBindings* bindings, AnScopeLocation parent, bool function);
AnBindingId an_add_binding(AnScopeBindings* bindings, AnScopeId scope, AnBinding binding);
AnBinding const* an_get_binding(AnScopeBindings bindings, AnBindingId id);
AnBinding* an_get_binding_mut(AnScopeBindings* bindings, AnBindingId id);

int an_find_binding_in_scope(AnScopeBindings bindings, AnScopeLocation loc, String s, OUT(AnBindingId) id);
int an_find_binding(AnScopeBindings bindings, AnScopeLocation loc, String s, OUT(AnBindingId) id);

AnScopeLocation an_scope_end_location(AnScopeBindings bindings, AnScopeId scope);
AnScopeLocation an_scope_constant_location(AnScopeId scope);

typedef struct AnBindingLocation {
    AnScopeLocation location;
    AnBindingId binding;
} AnBindingLocation;

typedef IdHashTable /* expression (reference) -> `AnBindingLocation` */
    AnReferenceBindingLocations;

typedef IdHashTable /* pattern (variable) -> `AnBindingId` */ AnVariableBindings;

typedef AstBindingSetId* AnPatternBindingSets;
