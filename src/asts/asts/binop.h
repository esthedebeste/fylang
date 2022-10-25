#pragma once
#include "../asts.h"

LLVMValueRef gen_num_num_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               NumType *lhs_nt, NumType *rhs_nt);
LLVMValueRef gen_ptr_num_binop(int op, LLVMValueRef ptr, LLVMValueRef num,
                               PointerType *ptr_t, NumType *num_t);
LLVMValueRef gen_ptr_ptr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               PointerType *lhs_ptr, PointerType *rhs_pt);
LLVMValueRef gen_arr_arr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               ArrayType *lhs_at, ArrayType *rhs_at);
Type *get_binop_type(int op, Type *lhs_t, Type *rhs_t);
Value *gen_binop(int op, LLVMValueRef L, LLVMValueRef R, Type *lhs_t,
                 Type *rhs_t);

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST *LHS, *RHS;

public:
  BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};