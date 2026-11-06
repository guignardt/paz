#pragma once

#include "data/bytecode.h"

typedef enum VmFlow {
    VM_FLOW_CONTINUE = 0,
    VM_FLOW_EXIT = 1,
} VmFlow;

typedef struct VmSystem {
    VmFlow(*exit)(int64_t code);
    VmFlow(*debug)(BytecodeVar var_index, Word value);
} VmSystem;

typedef struct VmFrame {
    BytecodeLoc _return_location;
    size_t _previous_variable_stack_bottom;
} VmFrame;

typedef struct Vm {
    VmSystem _system;
    Bytecode _bytecode;
    uint8_t const* _ip;
    int64_t _exit_code;
    DYNAMIC_ARRAY(VmFrame) _frame_stack;
    DYNAMIC_ARRAY(Word) _expression_stack;
    DYNAMIC_ARRAY(Word) _variable_stack;
    size_t _variable_stack_bottom;
} Vm;

Vm vm_new(Bytecode bytecode, VmSystem system);
int64_t vm_run(Vm* vm, int64_t input);
void vm_free(OWNED(Vm) vm);

VmSystem default_vm_system(void);
