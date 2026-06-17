#pragma once

#include <stddef.h>

typedef struct String {
    char const* data;
    size_t len;
} String;

BOXED(char) format(char const* fmt, ...);
