#include "assign.h"

AssignExprAST::AssignExprAST(ExprAST *LHS, ExprAST *RHS) : LHS(LHS), RHS(RHS) {}
Type *AssignExprAST::get_type() { return LHS->get_type(); }
Value *AssignExprAST::gen_value() {
  CastValue *val = RHS->gen_value()->cast_to(LHS->get_type());
  LLVMBuildStore(curr_builder, val->gen_val(), LHS->gen_value()->gen_ptr());
  return val;
}