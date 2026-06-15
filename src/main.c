#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "diagnostic/log.h"

#include "data/source.h"
#include "data/token.h"

#include "phase/tokenize.h"

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

    eprintf("foo %s\n", "bar");
    log(SEVERITY_ERROR, "an error");
    log(SEVERITY_WARNING, "im %s ya", "warning");
    log(SEVERITY_INFO, "i think 2 + 2 = %d", 4);
    log (SEVERITY_DEBUG, "caught some bugs yet?");

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

    for (size_t i = 0; i < tokens._len; i++) {
        eprintf("%zu: %zu\n", i, ((size_t const*)(tokens._elements))[i]);
    }
    eprintf("\n");

    TokenIt it0 = token_stream_it(tokens);

    TokenTree hey; assert(!token_it_next(&it0, &hey));
    assert(hey.kind == TOKEN_TREE_SINGLE);
    assert(hey.as.single.kind == TOKEN_IDENTIFIER);
    eprintf("'hey' range: %zu-%zu\n", hey.as.single.range.start, hey.as.single.range.end);

    TokenTree paren1; assert(!token_it_next(&it0, &paren1));
    assert(paren1.kind == TOKEN_TREE_GROUP);
    assert(paren1.as.group.delim == TOKEN_DELIM_PAREN);
    eprintf("outer paren left range: %zu-%zu\n", paren1.as.group.left.start, paren1.as.group.left.end);
    eprintf("outer paren right range: %zu-%zu\n", paren1.as.group.right.start, paren1.as.group.right.end);

    TokenTree foo; assert(!token_it_next(&it0, &foo));
    assert(foo.kind == TOKEN_TREE_SINGLE);
    assert(foo.as.single.kind == TOKEN_IDENTIFIER);
    eprintf("'foo' range: %zu-%zu\n", foo.as.single.range.start, foo.as.single.range.end);

    assert(token_it_get(it0, NULL));

    TokenIt it1 = token_group_contents(paren1.as.group);

    TokenTree im; assert(!token_it_next(&it1, &im));
    assert(im.kind == TOKEN_TREE_SINGLE);
    assert(im.as.single.kind == TOKEN_IDENTIFIER);
    eprintf("'im' range: %zu-%zu\n", im.as.single.range.start, im.as.single.range.end);

    TokenTree in; assert(!token_it_next(&it1, &in));
    assert(in.kind == TOKEN_TREE_SINGLE);
    assert(in.as.single.kind == TOKEN_IDENTIFIER);
    eprintf("'in' range: %zu-%zu\n", in.as.single.range.start, in.as.single.range.end);

    TokenTree paren2; assert(!token_it_next(&it1, &paren2));
    assert(paren2.kind == TOKEN_TREE_GROUP);
    assert(paren2.as.group.delim == TOKEN_DELIM_PAREN);
    eprintf("inner paren left range: %zu-%zu\n", paren2.as.group.left.start, paren2.as.group.left.end);
    eprintf("inner paren right range: %zu-%zu\n", paren2.as.group.right.start, paren2.as.group.right.end);

    assert(token_it_get(it1, NULL));

    TokenIt it2 = token_group_contents(paren2.as.group);
    
    TokenTree parens; assert(!token_it_next(&it2, &parens));
    assert(parens.kind == TOKEN_TREE_SINGLE);
    assert(parens.as.single.kind == TOKEN_IDENTIFIER);
    eprintf("'parens' range: %zu-%zu\n", parens.as.single.range.start, parens.as.single.range.end);

    assert(token_it_get(it2, NULL));

    token_stream_free(&tokens);
    source_free(&source);

    return 0;
}
