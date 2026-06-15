#include "diagnostic/report.h"

static Reporter global_reporter = {
    .begin = NULL,
    .end = NULL,
    .message_raw = NULL,
};
static size_t global_report_count = 0;

void set_reporter(Reporter reporter) {
    global_reporter = reporter;
    global_report_count = 0;
}

size_t report_count(void) {
    return global_report_count;
}

void report_begin(Severity severity, int32_t code) {
    global_report_count++;
    void (*func)(Severity, int32_t) = global_reporter.begin;
    if (func) {
        (func)(severity, code);
    }
}

void report_message_raw(BOXED(char) message) {
    void (*func)(BOXED(char)) = global_reporter.message_raw;
    if (func) {
        (func)(message);
    } else {
        free(message);
    }
}

void report_end(void) {
    void (*func)() = global_reporter.end;
    if (func) {
        (func)();
    }
}

static void default_reporter_begin(Severity severity, int32_t code);
static void default_reporter_message_raw(BOXED(char) message);
static void default_reporter_end(void);

Reporter default_reporter(void) {
    return (Reporter) {
        .begin = default_reporter_begin,
        .message_raw = default_reporter_message_raw,
        .end = default_reporter_end,
    };
}

static Severity default_reporter_current_severity = SEVERITY_DEBUG;

static void default_reporter_begin(Severity severity, int32_t code) {
    default_reporter_current_severity = severity;
}

static void default_reporter_message_raw(BOXED(char) message) {
    log(default_reporter_current_severity, "%s", message);
}

static void default_reporter_end(void) {}
