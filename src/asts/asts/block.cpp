#include "block.h"
#include "../scope.h"

BlockExprAST::BlockExprAST(std::vector<ExprAST *> exprs) : exprs(exprs) {
  if (exprs.size() == 0)
    error("block can't be empty.");
}
Type *BlockExprAST::get_type() {
  push_scope();
  // initialize previous exprs
  for (size_t i = 0; i < exprs.size() - 1; i++)
    exprs[i]->get_type();
  Type *type = exprs.back()->get_type();
  pop_scope();
  return type;
}
Value *BlockExprAST::gen_value() {
  push_scope();
  // generate code for all exprs and only return last expr
  for (size_t i = 0; i < exprs.size() - 1; i++)
    exprs[i]->gen_value();
  Value *value = exprs.back()->gen_value();
  pop_scope();
  return value;
}