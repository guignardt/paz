#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "data/analysis/type.h"

AnTypes an_types_new(void) {
    return (AnTypes) { ._data = NULL, ._len = 0, ._capacity = 0 };
}

AnTypes an_types_default(void) {
    const AnType default_types[] = {
        { .kind = AN_TYPE_INVALID },
        { .kind = AN_TYPE_TYPE },
        { .kind = AN_TYPE_INT },
    };

    AnTypes types = an_types_new();
    reserve(ERASE2(&types._data), &types._capacity, sizeof(default_types), 1);
    memcpy(types._data, default_types, sizeof(default_types));
    types._len += sizeof(default_types) / sizeof(AnType);

    return types;
}

void an_types_free(OWNED(AnTypes) types) {
    free(types->_data);
}

AnType an_types_get(AnTypes types, AnTypeId id) {
    return types._data[id];
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
        case AN_TYPE_FLOW:
            return a.as.flow == b.as.flow;
    }
}

AnTypeId an_types_get_id_or_insert(AnTypes* types, AnType type) {
    for (AnTypeId i = 0; i < types->_len; i++) {
        AnType elt = types->_data[i];
        if (an_type_eq(elt, type)) return i;
    }

    AnTypeId id = types->_len;
    reserve(ERASE2(&types->_data), &types->_capacity, types->_len + 1, sizeof(AnType));
    types->_data[types->_len++] = type;
    return id;
}

void an_register_expression(AnExpressions* expressions, AstExpression const* v) {
    reserve(ERASE2(&expressions->data), &expressions->_capacity, expressions->len + 1, sizeof(v));
    expressions->data[expressions->len++] = v;
}
