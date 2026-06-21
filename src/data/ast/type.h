#pragma once

#include <stdint.h>

#include "data/ast/identifier.h"
#include "util/alloc.h"

typedef uint32_t TypeId;

typedef struct FunctionType {
    TypeId input;
    TypeId output;
} FunctionType;

typedef enum TypeKind {
    TYPE_TYPE,
    TYPE_INT,
    TYPE_FUNCTION,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    union {
        FunctionType function;
    } as;
} Type;

typedef struct TypeRegistry {
    BOXED(Type) _types;
    size_t _len;
    size_t _capacity;
} TypeRegistry;

TypeRegistry type_registry_new(void);
void type_registry_free(OWNED(TypeRegistry) registry);
Type type_registry_get(TypeRegistry registry, TypeId id);
TypeId type_registry_get_or_add(TypeRegistry* registry, Type type);

typedef enum AstTypeNameKind {
    AST_TYPE_NAME_IDENTIFIER
} AstTypeNameKind;

typedef struct AstTypeName {
    // node
    struct AstTypeName* next;
    Range range;

    AstTypeNameKind kind;
    union {
        AstIdentifier identifier;
    } as;
} AstTypeName;

void debug_ast_type_name(AstTypeName const* v);
