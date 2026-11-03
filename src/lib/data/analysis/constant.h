#pragma once

#include <stdint.h>

#include "data/analysis/function.h"
#include "data/analysis/id_hash_table.h"

typedef struct AnConstantTuple {
    BOXED(struct AnConstant) elements;
    size_t len;
} AnConstantTuple;

typedef enum AnConstantKind {
    AN_CONSTANT_INVALID,
    AN_CONSTANT_TYPE,
    AN_CONSTANT_FUNCTION,
    AN_CONSTANT_TUPLE,
} AnConstantKind;

typedef struct AnConstant {
    AnConstantKind kind;
    union {
        AnTypeId type;
        AnFunctionId function;
        AnConstantTuple tuple;
    } as;
    AnTypeId type;
} AnConstant;

typedef IdHashTable /* expression -> `AnConstant` */ AnConstants;
