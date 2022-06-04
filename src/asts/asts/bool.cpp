#include "../asts.h"

NumType bool_type = NumType(1, false, false);
BoolExprAST::BoolExprAST(bool value) : value(value) {}
Type *BoolExprAST::get_type() { return &bool_type; }
Value *BoolExprAST::gen_value() {
  return new ConstValue(&bool_type,
                        value ? LLVMConstAllOnes(bool_type.llvm_type())
                              : LLVMConstNull(bool_type.llvm_type()));
}
bool BoolExprAST::is_constant() { return true; }