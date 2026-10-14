#pragma once

#include <stdint.h>

#include "data/analysis/function.h"
#include "data/analysis/id_hash_table.h"

typedef enum AnConstantKind {
    AN_CONSTANT_INVALID,
    AN_CONSTANT_TYPE,
    AN_CONSTANT_FUNCTION,
} AnConstantKind;

typedef struct AnConstant {
    AnConstantKind kind;
    union {
        AnTypeId type;
        AnFunctionId function;
    } as;
    AnTypeId type;
} AnConstant;

typedef IdHashTable /* expression -> `AnConstant` */ AnConstants;
