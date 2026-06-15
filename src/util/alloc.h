#pragma once

#include <stdlib.h>

#define OUT(T) T*

// A variable of type BOXED(T) owns the value and is responsible for freeing the memory it which
// it lies.
#define BOXED(T) T*

// A variable of type OWNED(T) owns the value but is not responsible for freeing the memory in
// which it lies. Typically, a function taking in an OWNED(T) will destroy its pointee (including
// all memory that the value owns) but not the memory directly referenced by this pointer. 
#define OWNED(T) T*

#define ERASE(ptr) ((void*)(ptr))
#define ERASE2(ptr) ((void**)(ptr))

BOXED(void) alloc_or_exit(size_t size);
BOXED(void) realloc_or_exit(BOXED(void) data, size_t size);

// `*p_cap` in bytes, `len` in units of size `size`
void reserve(BOXED(void)* p_buf, size_t* p_cap, size_t len, size_t size);

typedef struct Range {
    size_t start;
    size_t end;
} Range;

BOXED(char) format(char const* fmt, ...);
