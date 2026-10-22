#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "phase/assemble.h"
#include "phase/emit.h"
#include "diagnostic/report.h"

typedef struct Assembler {
    Emitter emitter;
    DYNAMIC_ARRAY(String) symbol_names;
} Assembler;

typedef union {
    char string[8];
    uint64_t bits;
} Mnemo;

static void assemble_line(char const* line, Assembler* a);
static int parse_line(char const* line, Assembler* a, OUT(BytecodeOp) dst);
static int parse_int(char const** s, OUT(int64_t) dst);
static int parse_var(char const** s, OUT(BytecodeVar) dst);
static int parse_register_symbol(char const** s, Assembler* a, OUT(EmitterSymbol) dst);
static int parse_identifier(char const** s, OUT(String) dst);
static void skip_whitespace(char const** s);
static int find_opcode(String name, Mnemo const* table, size_t table_len, OUT(int) dst);

static int check_op(BytecodeOp op);
static void report_invalid_argument();

Bytecode assemble(char const* const* lines, size_t line_count) {
    Assembler a = {
        .emitter = emitter_new(),
        .symbol_names = {0}
    };

    for (size_t i = 0; i < line_count; i++) {
        assemble_line(lines[i], &a);
    }
    free(a.symbol_names.data);
    return emit_finish(&a.emitter);
}

#define PROC(mnemo, code, ...) [code] = { .string = #mnemo },
static Mnemo opcode_mnemos[] = { FOR_OPS };
static Mnemo syscall_mnemos[] = { FOR_SYSCALLS };
#undef PROC

static void assemble_line(char const* line, Assembler* a) {
    BytecodeOp op;
    if (parse_line(line, a, &op)) {
        return;
    }
    if (check_op(op)) {
        return;
    }
    emit_raw(&a->emitter, op);
}

static int parse_line(char const* line, Assembler* a, OUT(BytecodeOp) dst) {
    if (line[0] == '\0') return -1;

    char const* s = line;
    skip_whitespace(&s);

    if (s[0] == ':') {
        s++;
        EmitterSymbol sym;
        if (parse_register_symbol(&s, a, &sym)) return -1;
        if (emit_symbol_location(&a->emitter, sym)) {
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
            report_message("duplicate symbol");
            report_end();
        }
        return -1;
    }

    BytecodeOp op = { .arg.kind = BYTECODE_ARG_NONE };

    String opname;
    if (parse_identifier(&s, &opname)) {
        return -1;
    }
    int opcode_raw;
    if (find_opcode(opname, opcode_mnemos, sizeof(opcode_mnemos) / sizeof(Mnemo), &opcode_raw)) {
        return -1;
    }
    op.opcode = opcode_raw;

    skip_whitespace(&s);
    if (s[0] == '\0') {
        *dst = op;
        return 0;
    }

    if (op.opcode == OP_SYS) {
        String syscall_name;
        if (parse_identifier(&s, &syscall_name)) {
            return -1;
        }
        int syscall_raw;
        if (find_opcode(syscall_name, syscall_mnemos, sizeof(syscall_mnemos) / sizeof(Mnemo), &syscall_raw)) {
            return -1;
        }
        op.syscall = syscall_raw;
    }

    skip_whitespace(&s);
    if (s[0] == '\0') {
        *dst = op;
        return 0;
    }

    EmitterSymbol symbol;
    switch (s[0]) {
        case '#':
            s++;
            op.arg.kind = BYTECODE_ARG_INT;
            if (parse_int(&s, &op.arg.as.int_)) return -1;
            break;
        
        case ':':
            s++;
            op.arg.kind = BYTECODE_ARG_LOC;
            if (parse_register_symbol(&s, a, &symbol)) return -1;
            op.arg.as.loc = symbol;
            break;

        case '%':
            s++;
            op.arg.kind = BYTECODE_ARG_VAR;
            if (parse_var(&s, &op.arg.as.var)) return -1;
            break;
        
        default: break;
    }

    skip_whitespace(&s);
    if (s[0] != '\0') {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("unexpected character after instruction");
        report_end();
        return -1;
    }

    *dst = op;
    return 0;
}

static int parse_int(char const** s, OUT(int64_t) dst) {
    if (**s == '\0') {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("invalid integer literal");
        report_end();
        return -1;
    }

    char* end; // not const just for strtoll
    errno = 0;
    int64_t val = strtoll(*s, &end, 0);
    if (errno) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("invalid integer literal");
        report_end();
        return -1;
    }

    *s = end;
    *dst = val;
    return 0;
}

static int parse_var(char const** s, OUT(BytecodeVar) dst) {
    if (**s == '\0') {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("expected variable number");
        report_end();
        return -1;
    }

    char* end; // not const just for strtol
    errno = 0;
    long val = strtol(*s, &end, 10);
    if (errno || val < 0 || val >= UINT16_MAX) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("invalid variable number");
        report_end();
        return -1;
    }

    *s = end;
    *dst = val;
    return 0;
}

static int parse_register_symbol(char const** s, Assembler* a, OUT(EmitterSymbol) dst) {
    String name;
    if (parse_identifier(s, &name)) return -1;

    EmitterSymbol sym = -1;
    for (size_t i = 0; i < a->symbol_names.len; i++) {
        String name2 = a->symbol_names.data[i];
        if (name.len == name2.len && !strncmp(name.data, name2.data, name.len)) {
            sym = i;
            break;
        }
    }

    if (sym == -1) {
        sym = emit_symbol(&a->emitter); // symbol == symbol_names len
        DYNAMIC_ARRAY_PUSH(&a->symbol_names, name);
    }

    *dst = sym;
    return 0;
}

static int parse_identifier(char const** s, OUT(String) dst) {
    if (!isalpha(**s)) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("expected identifier");
        report_end();
        return -1;
    }
    
    char const* start = *s;
    char const* end = start;
    while (isalnum(*end) || *end == '_') {
        end++;
    }
    *s = end;
    size_t len = end - start;
    *dst = (String) { .data = start, .len = len };
    return 0;
}


static void skip_whitespace(char const** s) {
    while (isspace(**s)) (*s)++;
}

static int find_opcode(String name, Mnemo const* table, size_t table_len, OUT(int) dst) {
    if (name.len > 7) goto find_opcode_not_found;

    Mnemo mnemo = {0};
    strncpy(mnemo.string, name.data, name.len);

    int i = 0;
    for (; i < table_len; i++) {
        Mnemo mnemo2 = table[i];
        if (mnemo.bits == mnemo2.bits) break;
    }
    if (i < table_len) {
        *dst = i;
        return 0;
    }

find_opcode_not_found:
    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
    report_message("unknown mnemonic %.*s", (int)name.len, name.data);
    report_end();
    return -1;
}

static int check_op(BytecodeOp op) {
    #define PROC(mnemo, code, ...)                                      \
        case code:                                                      \
            __VA_OPT__(                                                 \
                if (op.arg.kind != BYTECODE_ARG_KIND(__VA_ARGS__)) {    \
                    report_invalid_argument();                          \
                    return -1;                                          \
                }                                                       \
                break;                                                  \
            )                                                           \
            if (op.arg.kind != BYTECODE_ARG_NONE) {                     \
                report_invalid_argument();                              \
                return -1;                                              \
            }                                                           \
            break;
    
    switch (op.opcode) {
        FOR_OPS
        
        case OP_SYS:
            switch (op.syscall) {
                FOR_SYSCALLS
            }
    }

    return 0;
}

static void report_invalid_argument() {
    report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
    report_message("invalid argument");
    report_end();
}
