#include "testing.h"

static unsigned int total = 0;
static unsigned int passed = 0;

void run_test(int(*func)(void), char const* name) {
    eprintf("\nrunning test `%s`...\n", name);
    total++;
    if (func()) {
        eprintf("test `%s` failed\n", name);
    } else {
        eprintf("test `%s` passed\n", name);
        passed++;
    }
}

void finish_tests(void) {
    eprintf("\nfinished running tests: %u/%u passed\n", passed, total);
}
