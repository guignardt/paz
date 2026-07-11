#include "data/analysis.h"

AnFunctionId an_functions_register(AnFunctions* nodes, AnFunction v) {
    AnFunctionId id = nodes->functions_len;
    reserve(ERASE2(&nodes->functions), &nodes->_functions_capacity, nodes->functions_len + 1, sizeof(v));
    nodes->functions[nodes->functions_len++] = v;
    return id;
}
