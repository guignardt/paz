#pragma once

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
