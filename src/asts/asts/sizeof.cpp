#include "sizeof.h"

NumType sizeof_type;
bool sizeof_type_init = false;

SizeofExprAST::SizeofExprAST(TypeAST *type) : type(type) {
  if (!sizeof_type_init) {
    sizeof_type = NumType(false); // uint_ptrsize
    sizeof_type_init = true;
  }
}
Type *SizeofExprAST::get_type() { return &sizeof_type; }
Value *SizeofExprAST::gen_value() {
  return new ConstValue(&sizeof_type, LLVMSizeOf(type->llvm_type()));
}
bool SizeofExprAST::is_constant() { return true; }