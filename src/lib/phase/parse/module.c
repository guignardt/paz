#include "phase/parse.h"

ParseStatus parse_module(Parser p, OUT(AstModule) dst) {
    AstModule v = { .binding_sets = ID_SENTINEL };
    AstBindingSetId previous = ID_SENTINEL;
    while (true) {
        while (!token_it_match_single(p.tokens, TOKEN_SEMICOLON, NULL)) {}
        if (token_it_end(*p.tokens)) {
            break;
        }

        AstBindingSetId binding;
        if (parse_binding_set(p, &binding)) {
            token_it_find_single(p.tokens, TOKEN_SEMICOLON, NULL);
        }
        if (binding != ID_SENTINEL) {
            if (previous == ID_SENTINEL) {
                v.binding_sets = binding;
            } else {
                p.nodes->binding_sets[previous].next = binding;
            }
        }
        previous = binding;
    }

    *dst = v;
    return PARSE_OK;
}
