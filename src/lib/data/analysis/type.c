#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "data/analysis/type.h"

AnTypes an_types_new(void) {
    return (AnTypes) { ._types = {0} };
}

AnTypes an_types_default(void) {
    const AnType default_types[] = {
        { .kind = AN_TYPE_INVALID },
        { .kind = AN_TYPE_TYPE },
        { .kind = AN_TYPE_INT },
    };

    AnTypes types = an_types_new();
    for (size_t i = 0; i < sizeof(default_types) / sizeof(AnType); i++) {
        DYNAMIC_ARRAY_PUSH(&types._types, default_types[i]);
    }

    return types;
}

void an_types_free(OWNED(AnTypes) types) {
    free(types->_types.data);
}

AnType an_types_get(AnTypes types, AnTypeId id) {
    return types._types.data[id];
}

bool an_type_eq(AnType a, AnType b) {
    if (a.kind != b.kind) return false;
    switch (a.kind) {
        case AN_TYPE_INVALID:
        case AN_TYPE_TYPE:
        case AN_TYPE_INT:
            return true;
        case AN_TYPE_FUNCTION:
            return a.as.function.input == b.as.function.input
                && a.as.function.output == b.as.function.output;
    }
}

AnTypeId an_types_get_id_or_insert(AnTypes* types, AnType type) {
    for (AnTypeId i = 0; i < types->_types.len; i++) {
        AnType elt = types->_types.data[i];
        if (an_type_eq(elt, type)) return i;
    }

    AnTypeId id = types->_types.len;
    DYNAMIC_ARRAY_PUSH(&types->_types, type);
    return id;
}
