#include "declare.h"

DeclareExprAST::DeclareExprAST(LetExprAST *let) : let(let) {
  curr_scope->declare_variable(let->id, let->get_type());
}
DeclareExprAST::DeclareExprAST(FunctionAST *func) : func(func) { func->add(); }
LLVMValueRef DeclareExprAST::gen_toplevel() {
  if (let)
    return let->gen_declare();
  return nullptr;
}