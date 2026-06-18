#include "diagnostic/log.h"

char const* severity_message[] = {
    [SEVERITY_ERROR] =      "\x1b[31merror: \x1b[0m",    // red
    [SEVERITY_WARNING] =    "\x1b[33mwarning: \x1b[0m",  // yellow
    [SEVERITY_INFO] =       "\x1b[32minfo: \x1b[0m",     // green
    [SEVERITY_DEBUG] =      "\x1b[36mdebug: \x1b[0m",    // cyan
    [SEVERITY_NONE] =       ""
};
