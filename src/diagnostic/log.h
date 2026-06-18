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

extern char const* severity_message[5];

#define log(severity, fmt, ...) \
    eprintf("%s" fmt "\n", severity_message[severity] __VA_OPT__(, __VA_ARGS__))
