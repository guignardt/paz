#include <stdlib.h>

#include "data/ast/nodes.h"

AstNodes ast_nodes_new(void) {
    return (AstNodes) {
        .expressions = NULL,
        .expressions_len = 0,
        ._expressions_cap = 0,

        .patterns = NULL,
        .patterns_len = 0,
        ._patterns_cap = 0,

        .binding_sets = NULL,
        .binding_sets_len = 0,
        ._binding_sets_cap = 0,
    };
}

void ast_nodes_free(AstNodes* nodes) {
    free(nodes->expressions);
    free(nodes->patterns);
    free(nodes->binding_sets);
}
