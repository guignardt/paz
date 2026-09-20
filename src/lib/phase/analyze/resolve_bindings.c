#include "phase/analyze.h"

#include "diagnostic/report.h"

void analyze_resolve_bindings(Analysis* a, AstNodes ast) {
    AstExpressionId reference_id;
    AnBindingLocation* binding_location;
    for (
        IdHashTableIt it = id_hash_table_it(a->reference_binding_locations);
        (binding_location = (AnBindingLocation*)id_hash_table_it_next(&it, &reference_id));
    ) {
        AnBindingId binding_id;
        String name = ast.expressions.data[reference_id].as.reference.identifier.string;
        if (an_find_binding(a->scope_bindings, binding_location->location, name, &binding_id)) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("failed to find binding with name `%.*s`", (int)name.len, name.data);
            report_end();
            continue;
        }
        binding_location->binding = binding_id;
    }
}
