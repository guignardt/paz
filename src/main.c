#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "diagnostic/report.h"

#include "data/source.h"
#include "data/token.h"

#include "phase/tokenize.h"

static void debug_tokens(TokenIt it, size_t indent);

int main(int argc, char const** argv) {
    set_reporter(default_reporter());

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

    printf("=== source code ===\n");
    printf("%s\n", source.text);
    printf("=== line indices ===\n");
    for (size_t i = 0; i < source.num_lines; i++) {
        printf("%zu ", source.line_indices[i]);
    }
    printf("\n\n");

    TokenStream tokens;
    tokenize(source.text, &tokens);
    
    debug_tokens(token_stream_it(tokens), 0);

    token_stream_free(&tokens);
    source_free(&source);

    return 0;
}

static void debug_tokens(TokenIt it, size_t indent) {
    TokenTree token_tree;
    while (!token_it_next(&it, &token_tree)) {
        switch (token_tree.kind) {
            case TOKEN_TREE_SINGLE:
                eprintf("%*stoken kind=%d range=%zu-%zu\n",
                    4 * (int)indent, "",
                    (int)token_tree.as.single.kind,
                    token_tree.as.single.range.start, token_tree.as.single.range.end
                );
                break;
            case TOKEN_TREE_GROUP:
                eprintf("%*sgroup delim=%d range=%zu-%zu\n",
                    4 * (int)indent, "",
                    (int)token_tree.as.group.delim,
                    token_tree.as.group.left.start,
                    token_tree.as.group.right.end
                );
                debug_tokens(token_group_contents(token_tree.as.group), indent + 1);
                break;
        }
    }
}
