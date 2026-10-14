#include <stdlib.h>

#include "data/analysis.h"

Analysis analysis_new(AstNodeView root) {
    AstNodes ast = ast_node_view_nodes(root);

    BOXED(AstBindingSetId) pattern_binding_sets = alloc_or_exit(ast.patterns.len * sizeof(AstBindingSetId));
    for (AstPatternId i = 0; i < ast.patterns.len; i++) {
        pattern_binding_sets[i] = ID_SENTINEL;
    }
    BOXED(AnTypeId) expression_types = alloc_or_exit(ast.expressions.len * sizeof(AnTypeId));
    for (AstPatternId i = 0; i < ast.expressions.len; i++) {
        expression_types[i] = ID_SENTINEL;
    }
    BOXED(AstBindingSetId) pattern_types = alloc_or_exit(ast.patterns.len * sizeof(AnTypeId));
    for (AstPatternId i = 0; i < ast.patterns.len; i++) {
        pattern_types[i] = ID_SENTINEL;
    }

    return (Analysis) {
        .root = root,
        .functions = {0},
        .function_ids = id_hash_table_new(sizeof(AnFunctionId)),
        .scope_bindings = an_scope_bindings_new(),
        .reference_binding_locations = id_hash_table_new(sizeof(AnBindingLocation)),
        .variable_bindings = id_hash_table_new(sizeof(AnBindingId)),
        .pattern_binding_sets = pattern_binding_sets,
        .constants = id_hash_table_new(sizeof(AnConstant)),
        .types = an_types_default(),
        .expression_types = expression_types,
        .pattern_types = pattern_types,
    };
}
