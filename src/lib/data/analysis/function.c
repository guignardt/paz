#include "data/analysis.h"

AnFunctionId an_functions_register(AnFunctions* functions, AnFunction v) {
    AnFunctionId id = functions->len;
    DYNAMIC_ARRAY_PUSH(functions, v);
    return id;
}
