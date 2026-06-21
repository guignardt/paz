#include "phase/parse.h"

ParseStatus parse_program(Parser p, OUT(AstProgram*) dst) {
    AstProgram v;
    ParseStatus status = parse_module(p, &v.root);
    AstProgram* ptr = ast_storage_alloc(p.storage, sizeof(AstProgram));
    *ptr = v;
    if (dst) *dst = ptr;
    return status;
}

ParseStatus parse_module(Parser p, OUT(AstModule*) dst) {
    AstModule v = { .bindings = NULL };
    AstBinding** write_next = &v.bindings;
    while (true) {
        while (!token_it_match_single(p.tokens, TOKEN_SEMICOLON, NULL)) {}
        if (token_it_end(*p.tokens)) {
            break;
        }

        AstBinding* binding;
        if (parse_binding(p, &binding)) {
            token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
        }
        if (binding) {
            *write_next = binding;
            write_next = &binding->next;
        }
    }

    AstModule* ptr = ast_storage_alloc(p.storage, sizeof(AstModule));
    *ptr = v;
    if (dst) *dst = ptr;
    return PARSE_OK;
}
