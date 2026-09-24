#include "data/ast.h"

AstNodes ast_node_view_nodes(AstNodeView v) {
    switch (v.kind) {
        case AST_NODE_MODULE:
            return v.as.module.nodes;
        case AST_NODE_EXPRESSION:
            return v.as.expression.nodes;
        case AST_NODE_PATTERN:
            return v.as.pattern.nodes;
        case AST_NODE_BINDING_SET:
            return v.as.binding_set.nodes;
    }
}
