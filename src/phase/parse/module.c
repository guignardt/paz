#include "phase/parse.h"

ParseStatus parse_program(Parser p, OUT(AstProgram) dst) {
    AstModule root;
    ParseStatus status = parse_module(p, &root);
    *dst = (AstProgram) { .root = root };
    return status;
}

ParseStatus parse_module(Parser p, OUT(AstModule) dst) {
    AstModule module = {0};
    AstBinding** write_next = &module.bindings;
    while (true) {
        while (!token_it_match_single(p.tokens, TOKEN_SEMICOLON, NULL)) {}
        if (token_it_end(*p.tokens)) {
            break;
        }

        AstBinding binding;
        switch (parse_binding(p, &binding)) {
            case PARSE_OK:
                if (token_it_match_single(p.tokens, TOKEN_SEMICOLON, NULL)) {
                    token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
                }
                break;
            case PARSE_ILL:
                token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
                break;
            case PARSE_ERROR:
                token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
                continue;
        }

        AstBinding* ptr = ast_storage_alloc(p.storage, sizeof(AstBinding));
        *ptr = binding;
        *write_next = ptr;
        write_next = &ptr->next;
    }

    if (dst) {
        *dst = module;
    }
    return PARSE_OK;
}
