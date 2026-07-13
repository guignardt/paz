#include "data/analysis.h"

Analysis analysis_new(void) {
    return (Analysis) {
        .functions = { .data = NULL, .len = 0, ._capacity = 0 },
        .function_ids = { .table = ptr_hash_table_new(sizeof(AnFunctionId)) },
        .scope_bindings = { .scopes = NULL, .scopes_len = 0, ._scopes_capacity = 0 },
        .expression_scopes = { .table = ptr_hash_table_new(sizeof(AnScopeId)) },
        .binding_ids = { .table = ptr_hash_table_new(sizeof(AnBindingId)) },
        .constant_bindings = { .data = NULL, .len = 0, ._capacity = 0 },
        .expressions = { .data = NULL, .len = 0, ._capacity = 0 },
        .expression_types = { .table = ptr_hash_table_new(sizeof(AnTypeId)) },
    };
}
