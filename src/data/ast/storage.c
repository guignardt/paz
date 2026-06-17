#include <stdlib.h>

#include "data/ast/storage.h"

#define AST_STORAGE_FIRST_ARENA_SIZE (1024)

// must be aligned to pointer (max alignment for all objects we actually create)
typedef struct Header {
    void* next;
} Header;

AstStorage ast_storage_new(void) {
    return (AstStorage) {
        ._top = NULL,
        ._top_capacity = 0,
        ._top_offset = 0,
    };
}

#define ALIGN_OFFSET (sizeof(void*) * sizeof(char) - 1)
#define ALIGN_MASK (~ALIGN_OFFSET)

void* ast_storage_alloc(AstStorage* storage, size_t size) {
    // align the size
    size = (size + ALIGN_OFFSET) & ALIGN_MASK;

    if (size <= storage->_top_capacity - storage->_top_offset) {
        void* ptr = storage->_top + storage->_top_offset;
        storage->_top_offset += size;
        return ptr;
    }

    // allocate a new arena
    size_t grow_cap = (storage->_top_capacity == 0)
        ? AST_STORAGE_FIRST_ARENA_SIZE
        : 2 * storage->_top_capacity;
    size_t cap = (grow_cap < size + sizeof(Header)) ? size + sizeof(Header) : grow_cap;
    void* arena = alloc_or_exit(cap);
    
    Header header = { .next = storage->_top };
    *(Header*)arena = header;

    *storage = (AstStorage) {
        ._top = arena,
        ._top_capacity = cap,
        ._top_offset = sizeof(header),
    };
    return arena + sizeof(header);
}

void ast_storage_free(OWNED(AstStorage) storage) {
    void* cursor = storage->_top;
    while (cursor) {
        Header header = *(Header const*)cursor;
        free(cursor);
        cursor = header.next;
    }
}
