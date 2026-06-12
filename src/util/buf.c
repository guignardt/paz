#include "util/buf.h"

void reserve(BOXED(void)* p_buf, size_t* p_cap, size_t size) {
    BOXED(void) buf = *p_buf;
    size_t cap = *p_cap;

    if (size <= cap) {
        return;
    }

    size_t grow_cap = (cap <= 4) ? 8 : (2 * cap);
    size_t new_cap = (grow_cap <= size) ? size : grow_cap;
    BOXED(void) new_buf = realloc_or_exit(buf, new_cap);

    *p_buf = new_buf;
    *p_cap = new_cap;
}
