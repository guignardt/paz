#pragma once

#include "util/alloc.h"

typedef uint16_t IrRegisterId;
#define IR_REGISTER_ID_SENTINEL ((IrRegisterId)(-1))

typedef size_t IrFunctionId;

typedef size_t BlockId;

typedef enum IrInstructionKind {
    IR_INSTRUCTION_PHI,

    IR_INSTRUCTION_CALL,

    AST_INSTRUCTION_EQ,
    AST_INSTRUCTION_NE,
    AST_INSTRUCTION_LE,
    AST_INSTRUCTION_LT,
    AST_INSTRUCTION_GE,
    AST_INSTRUCTION_GT,

    IR_INSTRUCTION_ADD,
    IR_INSTRUCTION_SUB,
    IR_INSTRUCTION_MUL,
    IR_INSTRUCTION_DIV,
    IR_INSTRUCTION_REM,
} IrInstructionKind;

typedef struct IrPhiCase {
    BlockId block;
    IrRegisterId* value;
} IrPhiCase;

typedef struct IrPhi {
    size_t dst_len;
    size_t cases_len;
    IrRegisterId* dst;
    IrPhiCase* cases;
} IrPhi;

typedef struct IrCall {
    IrRegisterId* dst;
    size_t dst_len;
    IrFunctionId callee;
    IrRegisterId* input;
    size_t input_len;
} IrCall;

typedef struct IrOperation {
    IrRegisterId operands[4];
} IrOperation;

typedef struct IrInstruction {
    IrInstructionKind kind;
    union {
        IrPhi phi;
        IrCall call;
        IrOperation operation;
    } as;
} IrInstruction;

typedef struct IrGoto {
    BlockId destination;
    size_t phi_index;
} IrGoto;

typedef struct IrReturn {
    IrRegisterId* value;
    size_t len;
} IrReturn;

typedef enum IrBlockEndKind {
    IR_BLOCK_END_GOTO,
    IR_BLOCK_END_RETURN,
} IrBlockEndKind;

typedef struct IrBlockEnd {
    IrBlockEndKind kind;
    union {
        IrGoto goto_;
        IrReturn return_;
    } as;
} IrBlockEnd;

typedef struct IrBlock {
    IrInstruction* instructions;
    size_t instruction_len;
    IrBlockEnd end;
} IrBlock;

typedef struct IrFunction {
    IrRegisterId* input;
    size_t input_len;
    size_t output_len;
    IrBlock* blocks;
    size_t blocks_len;
} IrFunction;

typedef struct IrModule {
    IrFunction* functions;
    size_t functions_len;
} IrModule;
