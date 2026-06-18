#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "diagnostic/report.h"

#include "data/source.h"
#include "data/token.h"
#include "data/ast.h"

#include "phase/tokenize.h"
#include "phase/parse.h"

#define DEBUG_SOURCE_CODE   0
#define DEBUG_LINE_INDICES  0
#define DEBUG_TOKENS        1
#define DEBUG_AST           0

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
            log(SEVERITY_ERROR, "failed to open source file");
            return -1;
        }
    }

    Source source;
    if (source_load(path, input, &source)) {
        log(SEVERITY_ERROR, "failed to read source file");
        return -1;
    }
    if (!input_stdin) {
        fclose(input);
    }

#if DEBUG_SOURCE_CODE
    eprintf("=== source code ===\n");
    eprintf("%s\n", source.text);
#endif

#if DEBUG_LINE_INDICES
    eprintf("=== line indices ===\n");
    for (size_t i = 0; i < source.num_lines; i++) {
        eprintf("%zu ", source.line_indices[i]);
    }
    eprintf("\n");
#endif

    TokenStream tokens;
    tokenize(source.text, &tokens);

#if DEBUG_TOKENS
    eprintf("\n=== tokens ===\n");
    debug_tokens(token_stream_it(tokens));
#endif

    if (report_count(SEVERITY_ERROR) > 0) {
        goto tokenize_error_end;
    }

    AstStorage ast_storage = ast_storage_new();
    TokenIt token_it = token_stream_it(tokens);
    Parser parser = (Parser) {
        .text = source.text,
        .tokens = &token_it,
        .storage = &ast_storage,
    };
    AstProgram program;
    parse_program(parser, &program);

#if DEBUG_AST
    eprintf("\n=== AST ===\n");
    debug_ast_program(program);
#endif

    if (report_count(SEVERITY_ERROR) > 0) {
        goto parse_error_end;
    }

parse_error_end:
    ast_storage_free(&ast_storage);
tokenize_error_end:
    token_stream_free(&tokens);
    source_free(&source);

    return 0;
}
