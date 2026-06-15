#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

void reserve(BOXED(void)* p_buf, size_t* p_cap, size_t len, size_t size) {
    BOXED(void) buf = *p_buf;
    size_t cap = *p_cap;
    size_t size_bytes = len * size;

    if (size_bytes <= cap) {
        return;
    }

    size_t grow_cap = (cap <= 4) ? 8 : (2 * cap);
    size_t new_cap = (grow_cap <= size_bytes) ? size_bytes : grow_cap;
    BOXED(void) new_buf = realloc_or_exit(buf, new_cap);

    *p_buf = new_buf;
    *p_cap = new_cap;
}

BOXED(char) format(char const* fmt, ...) {
    va_list args0, args1;
    va_start(args0, fmt);
    va_copy(args1, args0);

    size_t len = vsnprintf(NULL, 0, fmt, args0);

    BOXED(char) buf = alloc_or_exit(len + 1);
    vsnprintf(buf, len + 1, fmt, args1);

    return buf;
}
