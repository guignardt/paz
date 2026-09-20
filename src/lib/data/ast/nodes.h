#pragma once

#include "util/alloc.h"
#include "util/id.h"

typedef Id AstExpressionId;
typedef Id AstPatternId;
typedef Id AstBindingSetId;

typedef struct AstNodes {
    DYNAMIC_ARRAY(struct AstExpression) expressions;
    DYNAMIC_ARRAY(struct AstPattern) patterns;
    DYNAMIC_ARRAY(struct AstBindingSet) binding_sets;
} AstNodes;

AstNodes ast_nodes_new(void);
void ast_nodes_free(AstNodes* nodes);
