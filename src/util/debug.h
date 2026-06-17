#pragma once

#include <stdbool.h>

#include "util/alloc.h"
#include "util/string.h"

// void debug_begin_children(void);
// void debug_end_children(void);
// void debug_begin_row(char const* name);
// void debug_attr_int(char const* attr, long long value);
// void debug_attr_range(char const* attr, Range range);
// void debug_attr_string(char const* attr, String str);
// void debug_end_row(void);

void debug_begin(char const* name);
void debug_end(void);
void debug_attr_begin(char const* name);
void debug_attr_end();
void debug_attr_int(char const* name, long long v);
void debug_attr_range(char const* name, Range range);
void debug_attr_string(char const* name, String string);
void debug_attr_begin_list(char const* name);
void debug_attr_end_list();
