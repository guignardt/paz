#include <stdlib.h>

#include "data/ast/nodes.h"

AstNodes ast_nodes_new(void) {
    return (AstNodes) {
        .expressions = {0},
        .patterns = {0},
        .binding_sets = {0},
    };
}

void ast_nodes_free(AstNodes* nodes) {
    free(nodes->expressions.data);
    free(nodes->patterns.data);
    free(nodes->binding_sets.data);
}
