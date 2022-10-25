#include "unary.h"
#include "binop.h"

UnaryExprAST::UnaryExprAST(int op, ExprAST *operand)
    : op(op), operand(operand) {}
Type *UnaryExprAST::get_type() {
  if (op == '*')
    if (PointerType *opt = dynamic_cast<PointerType *>(operand->get_type()))
      return opt->get_points_to();
    else
      error("* can't be used on a non-pointer type");
  else if (op == '&')
    return operand->get_type()->ptr();
  else
    return operand->get_type();
}
Value *UnaryExprAST::gen_value() {
  Type *type = get_type();
  Value *val = operand->gen_value();
  switch (op) {
  case '!':
    // shortcut for == 0
    if (auto num_type = dynamic_cast<NumType *>(val->get_type()))
      return new ConstValue(
          new NumType(1, false, false),
          gen_num_num_binop(T_EQEQ, val->gen_val(),
                            LLVMConstNull(val->get_type()->llvm_type()),
                            num_type, num_type));
    else if (auto ptr_type = dynamic_cast<PointerType *>(val->get_type()))
      return new ConstValue(
          new NumType(1, false, false),
          LLVMBuildICmp(curr_builder, LLVMIntEQ, val->gen_val(),
                        LLVMConstNull(val->get_type()->llvm_type()), UN));
    error("'!' unary op can only be used on numbers");
  case '-':
    // shortcut for 0 - num
    if (NumType *num_type = dynamic_cast<NumType *>(val->get_type()))
      return new ConstValue(
          num_type,
          gen_num_num_binop('-', LLVMConstNull(val->get_type()->llvm_type()),
                            val->gen_val(), num_type, num_type));
    else
      error("'-' unary op can only be used on numbers");
  case '~':
    if (NumType *num_type = dynamic_cast<NumType *>(val->get_type()))
      return new ConstValue(num_type,
                            LLVMBuildNot(curr_builder, val->gen_val(), UN));
    else
      error("'~' unary op can only be used on numbers");
  case '*':
    return new BasicLoadValue(type, val->gen_val());
  case '&':
    return new ConstValue(type, val->gen_ptr());
  case T_RETURN: {
    // from functions.h
    void add_return(Value * ret_val, LLVMBasicBlockRef curr_block);

    auto curr_block = LLVMGetInsertBlock(curr_builder);
    add_return(val, curr_block);
    LLVMPositionBuilderAtEnd(
        curr_builder,
        LLVMAppendBasicBlock(LLVMGetBasicBlockParent(curr_block), UN));
    return val;
  }
  default:
    error("invalid prefix unary operator '" + token_to_str(op) + "'");
  }
}
bool UnaryExprAST::is_constant() {
  if (op == T_RETURN || op == '*')
    return false;
  return operand->is_constant();
}