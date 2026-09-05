#pragma once

#include "diagnostic/log.h"

#define RUN_TEST(name) run_test(name, #name)

#define TEST_ASSERT(value) do {                         \
    if (!(value)) {                                     \
        eprintf("assertion failed: %s\n", #value);      \
        return -1;                                      \
    }                                                   \
} while(0)

void run_test(int(*func)(void), char const* name);
void finish_tests(void);
