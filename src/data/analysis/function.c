#include "data/analysis.h"

AnFunctionId an_functions_register(AnFunctions* nodes, AnFunction v) {
    AnFunctionId id = nodes->len;
    reserve(ERASE2(&nodes->data), &nodes->_capacity, nodes->len + 1, sizeof(v));
    nodes->data[nodes->len++] = v;
    return id;
}
