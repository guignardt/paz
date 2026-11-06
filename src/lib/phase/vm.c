#include <string.h>

#include "phase/vm.h"

#include "diagnostic/report.h"

#include "util/attr.h"

Vm vm_new(Bytecode bytecode, VmSystem system) {
    return (Vm) {
        ._system = system,
        ._bytecode = bytecode,
        ._ip = bytecode.data,
        ._exit_code = 0,
        ._frame_stack = {0},
        ._expression_stack = {0},
        ._variable_stack = {0},
    };
}

void vm_free(OWNED(Vm) vm) {
    free(vm->_frame_stack.data);
    free(vm->_expression_stack.data);
    free(vm->_variable_stack.data);
}

static VmFlow default_vm_exit(int64_t exit_code);
static VmFlow default_vm_debug(BytecodeVar var_index, Word value);

VmSystem default_vm_system(void) {
    return (VmSystem) {
        .exit = default_vm_exit,
        .debug = default_vm_debug,
    };
}

static VmFlow default_vm_exit(int64_t exit_code) {
    report_begin(SEVERITY_INFO, DUMMY_ERROR_CODE);
    report_message("program finished with exit code %lld", (long long)exit_code);
    report_end();
    return VM_FLOW_EXIT;
}

static VmFlow default_vm_debug(BytecodeVar var_index, Word value) {
    report_begin(SEVERITY_DEBUG, DUMMY_ERROR_CODE);
    report_message("variable %u had value %lld", (unsigned int)var_index, (long long)value.as_int);
    report_end();
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_run_one(Vm* vm);

#define PROC(mnemo, code, ...)                      \
    static VmFlow vm_op_##mnemo(                    \
        Vm* vm                                      \
        __VA_OPT__(, BytecodeArg(__VA_ARGS__) arg)  \
    );
FOR_OPS
#undef PROC

#define PROC(mnemo, code, ...)                      \
    static VmFlow vm_op_sys_##mnemo(                \
        Vm* vm                                      \
        __VA_OPT__(, BytecodeArg(__VA_ARGS__) arg)  \
    );
FOR_SYSCALLS
#undef PROC

static void vm_push(Vm* vm, Word v) {
    DYNAMIC_ARRAY_PUSH(&vm->_expression_stack, v);
}

static VmFlow vm_pop(Vm* vm, OUT(Word) v) {
    if (unlikely(vm->_expression_stack.len == 0)) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("vm error: tried to pop from empty expression stack");
        report_end();
        return VM_FLOW_EXIT;
    }
    *v = vm->_expression_stack.data[--vm->_expression_stack.len];
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_pop2(Vm* vm, OUT(Word) a, OUT(Word) b) {
    if (unlikely(vm->_expression_stack.len < 2)) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("vm error: tried to pop from empty expression stack");
        report_end();
        return VM_FLOW_EXIT;
    }
    *b = vm->_expression_stack.data[--vm->_expression_stack.len];
    *a = vm->_expression_stack.data[--vm->_expression_stack.len];
    return VM_FLOW_CONTINUE;
}

#define VM_POP(vm, v) do {                  \
    if (vm_pop(vm, v)) return VM_FLOW_EXIT; \
} while (0)                                 \

#define VM_POP2(vm, a, b) do {                  \
    if (vm_pop2(vm, a, b)) return VM_FLOW_EXIT; \
} while (0)                                     \

int64_t vm_run(Vm* vm, int64_t input) {
    vm_push(vm, (Word){ .as_int = input });
    while (vm_run_one(vm) == VM_FLOW_CONTINUE) {}
    return vm->_exit_code;
}

static VmFlow vm_run_one(Vm* vm) {
    BytecodeOp op = bytecode_read_op(&vm->_ip);
    switch (op.opcode) {
        #define PROC(mnemo, code, ...)                                             \
            case code:                                                             \
                return vm_op_##mnemo(vm __VA_OPT__(, op.arg.as.__VA_ARGS__##_));
        FOR_OPS
        #undef PROC

        case OP_SYS:
            switch (op.syscall) {
                #define PROC(mnemo, code, ...)                                              \
                    case code:                                                              \
                        return vm_op_sys_##mnemo(vm __VA_OPT__(, op.arg.as.__VA_ARGS__##_));
                FOR_SYSCALLS
                #undef PROC
            }
    }
}

static VmFlow vm_op_nop([[maybe_unused]] Vm* vm) {
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_call(Vm* vm) {
    Word loc; VM_POP(vm, &loc);
    VmFrame frame = {
        ._return_location = vm->_ip - vm->_bytecode.data,
        ._previous_variable_stack_bottom = vm->_variable_stack_bottom,
    };
    DYNAMIC_ARRAY_PUSH(&vm->_frame_stack, frame);
    vm->_ip = vm->_bytecode.data + loc.as_loc;
    vm->_variable_stack_bottom = vm->_variable_stack.len;
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_res(Vm* vm, BytecodeVar size) {
    size_t new_len = vm->_variable_stack.len + size;
    reserve(ERASE2(&vm->_variable_stack.data), &vm->_variable_stack.cap, new_len, sizeof(Word));
    memset(vm->_variable_stack.data + vm->_variable_stack.len, 0, size * sizeof(Word));
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_ret(Vm* vm) {
    if (vm->_frame_stack.len == 0) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("vm error: tried to return from outside of any function");
        report_end();
        return VM_FLOW_EXIT;
    }
    VmFrame frame = vm->_frame_stack.data[--vm->_frame_stack.len];
    vm->_ip = vm->_bytecode.data + frame._return_location;
    vm->_variable_stack_bottom = frame._previous_variable_stack_bottom;
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_jmp(Vm* vm, BytecodeLoc loc) {
    vm->_ip = vm->_bytecode.data + loc;
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_jmpz(Vm* vm, BytecodeLoc loc) {
    Word cond; VM_POP(vm, &cond);
    if (cond.as_int == 0) {
        vm->_ip = vm->_bytecode.data + loc;
    }
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_jmpnz(Vm* vm, BytecodeLoc loc) {
    Word cond; VM_POP(vm, &cond);
    if (cond.as_int != 0) {
        vm->_ip = vm->_bytecode.data + loc;
    }
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_int(Vm* vm, int64_t val) {
    vm_push(vm, (Word) { .as_int = val });
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_loc(Vm* vm, BytecodeLoc val) {
    vm_push(vm, (Word) { .as_loc = val });
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_var(Vm* vm, BytecodeVar var) {
    Word val = vm->_variable_stack.data[vm->_variable_stack_bottom + var];
    vm_push(vm, val);
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_set(Vm* vm, BytecodeVar var) {
    Word val; VM_POP(vm, &val);
    vm->_variable_stack.data[vm->_variable_stack_bottom + var] = val;
    return VM_FLOW_CONTINUE;
}

static VmFlow vm_op_negi(Vm* vm) {
    Word val; VM_POP(vm, &val);
    if (unlikely(val.as_int == INT64_MIN)) {
        report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);
        report_message("integer overflow: attempted to negate %lld", (long long)val.as_int);
        report_end();
        return VM_FLOW_EXIT;
    }
    val.as_int = -val.as_int;
    vm_push(vm, val);
    return VM_FLOW_CONTINUE;
}

#define VM_OP_COMPARE_INTS(mnemo, op)               \
    static VmFlow vm_op_##mnemo(Vm* vm) {           \
        Word a, b; VM_POP2(vm, &a, &b);             \
        int64_t result = -(a.as_int op b.as_int);   \
        vm_push(vm, (Word) { .as_int = result });   \
        return VM_FLOW_CONTINUE;                    \
    }
VM_OP_COMPARE_INTS(eqi, ==)
VM_OP_COMPARE_INTS(nei, !=)
VM_OP_COMPARE_INTS(lei, <=)
VM_OP_COMPARE_INTS(lti, <)
VM_OP_COMPARE_INTS(gei, >=)
VM_OP_COMPARE_INTS(gti, >)

#define VM_OP_RING_INTS(mnemo, builtin, ofmsg)                  \
    static VmFlow vm_op_##mnemo(Vm* vm) {                       \
        Word a, b; VM_POP2(vm, &a, &b);                         \
        int64_t result;                                         \
        if (unlikely(builtin(a.as_int, b.as_int, &result))) {   \
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE);     \
            report_message(ofmsg, a.as_int, b.as_int);          \
            report_end();                                       \
            return VM_FLOW_EXIT;                                \
        }                                                       \
        vm_push(vm, (Word) { .as_int = result });               \
        return VM_FLOW_CONTINUE;                                \
    }
VM_OP_RING_INTS(addi, __builtin_add_overflow, "integer overflow while computing %lld + %lld");
VM_OP_RING_INTS(subi, __builtin_sub_overflow, "integer overflow while computing %lld - %lld");
VM_OP_RING_INTS(muli, __builtin_mul_overflow, "integer overflow while computing %lld * %lld");

#define VM_OP_DIV_INTS(mnemo, op, msg)                      \
    static VmFlow vm_op_##mnemo(Vm* vm) {                   \
        Word a, b; VM_POP2(vm, &a, &b);                     \
        if (b.as_int == 0) {                                \
            report_begin(SEVERITY_ERROR, DUMMY_ERROR_CODE); \
            report_message(msg, a.as_int, b.as_int);        \
            report_end();                                   \
            return VM_FLOW_EXIT;                            \
        }                                                   \
        int64_t result = a.as_int op b.as_int;              \
        vm_push(vm, (Word) { .as_int = result });           \
        return VM_FLOW_CONTINUE;                            \
    }
VM_OP_DIV_INTS(divi, /, "integer division by zero while computing %lld / %lld");
VM_OP_DIV_INTS(remi, %, "integer division by zero while computing %lld %% %lld");

VmFlow vm_op_sys_nop([[maybe_unused]] Vm* vm) {
    return VM_FLOW_CONTINUE;
}

VmFlow vm_op_sys_exit(Vm* vm) {
    Word val; VM_POP(vm, &val);
    vm->_exit_code = val.as_int;
    (vm->_system.exit)(val.as_int);
    return VM_FLOW_EXIT;
}

VmFlow vm_op_sys_debug(Vm* vm, BytecodeVar var) {
    Word val = vm->_variable_stack.data[vm->_variable_stack_bottom + var];
    (vm->_system.debug)(var, val);
    return VM_FLOW_CONTINUE;
}
