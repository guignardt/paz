#include <stdlib.h>

#include "util/alloc.h"
#include "data/ast/type.h"
#include "diagnostic/report.h"

TypeRegistry type_registry_new(void) {
    size_t len = 2;
    size_t capacity = len * sizeof(Type);
    BOXED(Type) types = alloc_or_exit(capacity);
    types[0] = (Type){ .kind = TYPE_TYPE };
    types[1] = (Type){ .kind = TYPE_INT };

    return (TypeRegistry) {
        ._types = types,
        ._len = len,
        ._capacity = capacity,
    };
}

void type_registry_free(OWNED(TypeRegistry) registry) {
    free(registry->_types);
}

Type type_registry_get(TypeRegistry registry, TypeId id) {
    if (id >= registry._len) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("internal error: caught an out-of-bounds type id");
        report_end();
        exit(-1);
    }
    return registry._types[id];
}

static TypeId type_registry_add(TypeRegistry* registry, Type type) {
    // FIXME: check that type ids don't overflow
    TypeId id = registry->_len;
    reserve(ERASE2(&registry->_types), &registry->_capacity, registry->_len + 1, sizeof(Type));
    registry->_types[registry->_len++] = type;
    return id;
}

TypeId type_registry_get_or_add(TypeRegistry* registry, Type type) {
    for (TypeId id = 0; id < registry->_len; id++) {
        Type existing = registry->_types[id];
        if (existing.kind != type.kind) {
            continue;
        }
        switch (type.kind) {
            case TYPE_TYPE:
            case TYPE_INT:
                return id;
            case TYPE_FUNCTION:
                if (existing.as.function.input != type.as.function.input) {
                    continue;
                }
                if (existing.as.function.output != type.as.function.output) {
                    continue;
                }
                return id;
        }
    }

    return type_registry_add(registry, type);
}
