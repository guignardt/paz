#include <stdlib.h>

#include "data/analysis.h"

Analysis analysis_new(AstNodes nodes) {
    return (Analysis) {
        .functions = {0},
        .function_ids = { .table = id_hash_table_new(sizeof(AnFunctionId)) },
        .scope_bindings = an_scope_bindings_new(),
        .reference_binding_locations = { .table = id_hash_table_new(sizeof(AnBindingLocation)) },
        .variable_bindings = { .table = id_hash_table_new(sizeof(AnBindingId)) },
        .constant_items = {0},
        .types = an_types_default(),
        .expression_types = { .table = malloc(nodes.expressions.len * sizeof(AnTypeId)) },
    };
}
