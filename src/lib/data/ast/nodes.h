#pragma once

#include "util/alloc.h"
#include "util/id.h"

typedef Id AstExpressionId;
typedef Id AstPatternId;
typedef Id AstBindingSetId;

typedef struct AstNodes {
    BOXED(struct AstExpression) expressions;
    size_t expressions_len;
    size_t _expressions_cap;

    BOXED(struct AstPattern) patterns;
    size_t patterns_len;
    size_t _patterns_cap;

    BOXED(struct AstBindingSet) binding_sets;
    size_t binding_sets_len;
    size_t _binding_sets_cap;
} AstNodes;

AstNodes ast_nodes_new(void);
void ast_nodes_free(AstNodes* nodes);
