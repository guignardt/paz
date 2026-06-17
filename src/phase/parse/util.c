#include "phase/parse/util.h"
#include "diagnostic/report.h"

void unexpected_token(void) {
    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
    report_message("unexpected token");
    report_end();
}
