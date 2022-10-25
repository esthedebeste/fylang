#include "null.h"

ConstValue *null_value(Type *type) {
  return new ConstValue(type, LLVMConstNull(type->llvm_type()));
}
ConstValue *null_value() {
  return new ConstValue(&null_type, LLVMConstNull(null_type.llvm_type()));
}

NullExprAST::NullExprAST(Type *type) : type(type) {}
NullExprAST::NullExprAST() : type(&null_type) {}
Type *NullExprAST::get_type() { return type; }
Value *NullExprAST::gen_value() { return null_value(type); }
bool NullExprAST::is_constant() { return true; }