#include "phase/parse.h"

bool parse_status_returned(ParseStatus status) {
    return status == PARSE_OK || status == PARSE_ILL;
}

bool parse_status_ill(ParseStatus status) {
    return status == PARSE_ILL || status == PARSE_ERROR;
}
