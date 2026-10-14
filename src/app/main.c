#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "diagnostic/report.h"

#include "data/source.h"
#include "data/token.h"
#include "data/ast.h"

#include "phase/tokenize.h"
#include "phase/parse.h"
#include "phase/analyze.h"

#define DEBUG_SOURCE_CODE   0
#define DEBUG_LINE_INDICES  0
#define DEBUG_TOKENS        0
#define DEBUG_AST           1
#define DEBUG_ANALYSIS      1

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

    AstNodes ast_nodes = ast_nodes_new();
    TokenIt token_it = token_stream_it(tokens);
    Parser parser = (Parser) {
        .text = source.text,
        .tokens = &token_it,
        .nodes = &ast_nodes,
    };
    AstModule module;
    parse_module(parser, &module);

#if DEBUG_AST
    eprintf("\n=== AST ===\n");
    debug_ast_module((AstModuleView) {
        .data = &module,
        .nodes = ast_nodes,
    });
#endif

    if (report_count(SEVERITY_ERROR) > 0) {
        goto parse_error_end;
    }

    Analysis analysis = analysis_new((AstNodeView) {
        .kind = AST_NODE_MODULE,
        .as.module = (AstModuleView) {
            .data = &module,
            .nodes = ast_nodes,
        },
    });

#if DEBUG_ANALYSIS
    eprintf("\n=== ANALYSIS ===\n");
#endif

    analyze_prepare(&analysis, false);
    if (report_count(SEVERITY_ERROR) > 0) {
        goto analyze_error_end;
    }

#if DEBUG_ANALYSIS
    {
        eprintf("\n--- function ids ---\n");
        AstExpressionId key;
        AnFunctionId const* p_value;
        for (
            IdHashTableIt it = id_hash_table_it(analysis.function_ids);
            (p_value = id_hash_table_it_next(&it, &key));
        ) {
            eprintf("%u: %u\n", (unsigned int)key, (unsigned int)(*p_value));
        }
    }
#endif

    analyze_resolve_bindings(&analysis);
    if (report_count(SEVERITY_ERROR) > 0) {
        goto analyze_error_end;
    }

#if DEBUG_ANALYSIS
    {
        eprintf("\n--- references ---\n");
        AstExpressionId key;
        AnBindingLocation const* p_value;
        for (
            IdHashTableIt it = id_hash_table_it(analysis.reference_binding_locations);
            (p_value = id_hash_table_it_next(&it, &key));
        ) {
            AnBinding binding = *an_get_binding(analysis.scope_bindings, p_value->binding);
            eprintf(
                "reference %u -> `%.*s` declared in pattern %u\n",
                (unsigned int)key,
                (int)binding.name.len, binding.name.data,
                (unsigned int)binding.top_level_pattern
            );
        }
    }
#endif

    analyze_evaluate(&analysis);
    if (report_count(SEVERITY_ERROR) > 0) {
        goto analyze_error_end;
    }

#if DEBUG_ANALYSIS
    {
        eprintf("\n--- expression types ---\n");
        for (AstExpressionId i = 0; i < ast_nodes.expressions.len; i++) {
            eprintf("%u: %u\n", (unsigned int)i, (unsigned int)analysis.expression_types[i]);
        }
    }
#endif

analyze_error_end:
    // TODO: free analysis
parse_error_end:
    ast_nodes_free(&ast_nodes);
tokenize_error_end:
    token_stream_free(&tokens);
    source_free(&source);

    return 0;
}
