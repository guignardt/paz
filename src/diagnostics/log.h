#pragma once

#include <stdio.h>

#define eprintf(fmt, ...) fprintf(stderr, fmt __VA_OPT__(, __VA_ARGS__))

typedef enum Severity {
    SEVERITY_ERROR,
    SEVERITY_WARNING,
    SEVERITY_INFO,
    SEVERITY_DEBUG,
    SEVERITY_NONE,
} Severity;

static char const* severity_message[] = {
    [SEVERITY_ERROR] =      "\x1b[31merror: \x1b[0m",    // red
    [SEVERITY_WARNING] =    "\x1b[33mwarning: \x1b[0m",  // yellow
    [SEVERITY_INFO] =       "\x1b[32minfo: \x1b[0m",     // green
    [SEVERITY_DEBUG] =      "\x1b[36mdebug: \x1b[0m",    // cyan
    [SEVERITY_NONE] =       ""
};

#define log(severity, fmt, ...) \
    eprintf("%s" fmt "\n", severity_message[severity] __VA_OPT__(, __VA_ARGS__))
