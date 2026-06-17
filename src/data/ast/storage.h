#pragma once

#include "util/alloc.h"

typedef struct AstStorage {
    BOXED(void) _top;
    size_t _top_offset;
    size_t _top_capacity;
} AstStorage;

AstStorage ast_storage_new(void);
void* ast_storage_alloc(AstStorage* storage, size_t size);
void ast_storage_free(OWNED(AstStorage) storage);
