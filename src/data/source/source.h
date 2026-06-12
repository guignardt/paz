#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "util/buf.h"

typedef struct Source {
    char const* path;
    BOXED(char) text;
    BOXED(size_t) line_indices;
    size_t num_lines;
} Source;

void source_new(char const* path, BOXED(char) text, Source* dst);
int source_load(char const* path, FILE* file, Source* dst);
void source_free(OWNED(Source) source);
