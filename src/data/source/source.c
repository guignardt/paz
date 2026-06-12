#include <stdbool.h>

#include "data/source/source.h"

void source_new(char const* path, BOXED(char) text, Source* dst) {
    OWNED(size_t) line_indices = NULL;
    size_t num_lines = 0;
    size_t line_indices_capacity = 0;

    reserve(
        ERASE2(&line_indices),
        &line_indices_capacity,
        sizeof(size_t)
    );
    line_indices[0] = 0;
    num_lines++;

    char c;
    size_t i = 0;
    for (; c = text[i], c != '\0'; i++) {
        if (c == '\n') {
            reserve(
                ERASE2(&line_indices),
                &line_indices_capacity,
                (num_lines + 1) * sizeof(size_t)
            );
            line_indices[num_lines] = i + 1;
            num_lines++;
        }
    }

    reserve(
        ERASE2(&line_indices),
        &line_indices_capacity,
        num_lines + sizeof(size_t)
    );
    line_indices[num_lines] = i;
    num_lines++;

    *dst = (Source){
        .path = path,
        .text = text,
        .line_indices = line_indices,
        .num_lines = num_lines,
    };
}

int source_load(char const* path, FILE* file, Source* dst) {
    BOXED(char) text = NULL;
    size_t len = 0;
    size_t capacity = 0;

    while (true) {
        if (feof(file)) {
            break;
        }

        reserve(ERASE2(&text), &capacity, len + 1);
        len += fread(text + len, 1, capacity - len, file);
        if (ferror(file)) {
            free(text);
            return -1;
        }
    }

    reserve(ERASE2(&text), &capacity, len + 1);
    text[len] = 1;
    len++;

    source_new(path, text, dst);
    return 0;
}

void source_free(OWNED(Source) source) {
    free(source->text);
    free(source->line_indices);
}
