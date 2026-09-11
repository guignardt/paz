#include "data/analysis.h"

Analysis analysis_new(void) {
    return (Analysis) {
        .functions = { .data = NULL, .len = 0, ._capacity = 0 },
        .function_ids = { .table = id_hash_table_new(sizeof(AnFunctionId)) },
        .scope_bindings = { .scopes = NULL, .scopes_len = 0, ._scopes_capacity = 0 },
        .reference_scopes = { .table = id_hash_table_new(sizeof(AnScopeId)) },
        .binding_ids = { .table = id_hash_table_new(sizeof(AnBindingId)) },
        .constant_bindings = { .data = NULL, .len = 0, ._capacity = 0 },
        .expressions = { .data = NULL, .len = 0, ._capacity = 0 },
        .constants = { .table = id_hash_table_new(sizeof(AnConstant)) },
        .constant_items = { .data = NULL, .len = 0, ._capacity = 0 },
        .types = an_types_default(),
        .expression_types = { .table = id_hash_table_new(sizeof(AnTypeId)) },
    };
}
