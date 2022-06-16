#include "../asts.h"

ArrayExprAST::ArrayExprAST(std::vector<ExprAST *> elements)
    : elements(elements) {}
Type *ArrayExprAST::get_type() {
  return new ArrayType(elements[0]->get_type(), elements.size());
}
Value *ArrayExprAST::gen_value() {
  auto elem_type = elements[0]->get_type();
  auto type = get_type();
  auto arr = LLVMGetPoison(type->llvm_type());
  for (size_t i = 0; i < elements.size(); i++) {
    auto value = elements[i]->gen_value()->cast_to(elem_type)->gen_val();
    arr = LLVMBuildInsertValue(curr_builder, arr, value, i, UN);
  }
  return new ConstValue(type, arr);
}