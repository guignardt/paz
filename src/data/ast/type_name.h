#pragma once

#include <stdint.h>

#include "data/ast/identifier.h"
#include "util/alloc.h"

typedef enum AstTypeNameKind {
    AST_TYPE_NAME_REFERENCE
} AstTypeNameKind;

typedef struct AstTypeName {
    // node
    struct AstTypeName* next;
    Range range;

    AstTypeNameKind kind;
    union {
        AstIdentifier reference;
    } as;
} AstTypeName;

void debug_ast_type_name(AstTypeName const* v);
