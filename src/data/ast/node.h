#pragma once

#include "util/alloc.h"

typedef struct AstNode {
    struct AstNode* next;
    Range range;
} AstNode;

#define NODE(v) ((AstNode) { .next = val.next, .range = v.range })
