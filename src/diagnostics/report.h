#pragma once

#include <stdint.h>

#include "util/alloc.h"
#include "diagnostics/log.h"

#define DUMMY_ERROR_CODE (-1)

typedef struct Reporter {
    void (*begin)(Severity severity, int32_t code);
    void (*message_raw)(BOXED(char) message);
    void (*end)(void);
} Reporter;

void set_reporter(Reporter reporter);

void report_begin(Severity severity, int32_t code);
void report_message_raw(BOXED(char) message);
#define report_message(fmt, ...) \
    report_message_raw(format(fmt __VA_OPT__(, __VA_ARGS__)))
void report_end(void);

// number of reports since the last `set_reporter` call
size_t report_count(void);

// prints to stderr
Reporter default_reporter(void);
