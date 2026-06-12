#include <stdio.h>
#include <stdlib.h>

#include "util/alloc.h"

BOXED(void) alloc_or_exit(size_t size) {
    return realloc_or_exit(NULL, size);
}

BOXED(void) realloc_or_exit(BOXED(void) data, size_t size) {
    BOXED(void) ptr = realloc(data, size);
    if (!ptr) {
        fprintf(stderr, "failed to allocate\n");
        exit(-1);
    }
    return ptr;
}
