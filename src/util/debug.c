#include "util/debug.h"

#include "diagnostic/log.h"

int indent = 0;

void debug_begin(char const* name) {
    eprintf("%*s%s\n", indent, "", name);
    indent += 2;
}

void debug_end(void) {
    indent -= 2;
}

void debug_attr_begin(char const* name) {
    eprintf("%*s%s:\n", indent, "", name);
    indent += 2;
}

void debug_attr_end() {
    indent -= 2;
}

void debug_attr_int(char const* name, long long v) {
    eprintf("%*s%s: %lld\n", indent, "", name, v);
}

void debug_attr_range(char const* name, Range range) {
    eprintf("%*s%s: %u-%u\n",
        indent, "", name,
        (unsigned int)range.start, (unsigned int)range.end
    );
}

void debug_attr_string(char const* name, String string) {
    eprintf("%*s%s: %.*s\n",
        indent, "", name,
        (int)string.len, string.data
    );
}

void debug_attr_begin_list(char const* name) {
    debug_attr_begin(name);
}

void debug_attr_end_list() {
    debug_attr_end();
}
