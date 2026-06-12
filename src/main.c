#include <stdio.h>
#include <stdbool.h>

#include "data/source/source.h"

int main(int argc, char const** argv) {
    FILE* input;
    bool input_stdin = argc == 1;
    char const* path;
    if (input_stdin) {
        path = NULL;
        input = stdin;
    } else {
        path = argv[1];
        input = fopen(path, "r");
        if (input == NULL) {
            fprintf(stderr, "failed to open source file");
            return -1;
        }
    }

    Source source;
    if (source_load(path, input, &source)) {
        fprintf(stderr, "failed to read source file");
        return -1;
    }
    if (!input_stdin) {
        fclose(input);
    }

    printf("%s\n", source.text);
    printf("=== line indices ===\n");
    for (size_t i = 0; i < source.num_lines; i++) {
        printf("%zu ", source.line_indices[i]);
    }
    printf("\n");

    source_free(&source);

    return 0;
}
