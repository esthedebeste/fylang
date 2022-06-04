#include "../asts.h"

TupleExprAST::TupleExprAST(std::vector<ExprAST *> values) : values(values) {}

Type *TupleExprAST::get_type() {
  std::vector<Type *> types;
  for (auto &value : values)
    types.push_back(value->get_type());
  t_type = new TupleType(types);
  if (is_new)
    return t_type->ptr();
  else
    return t_type;
}

Value *TupleExprAST::gen_value() {
  if (is_constant()) {
    LLVMValueRef *vals = new LLVMValueRef[values.size()];
    for (size_t i = 0; i < values.size(); i++)
      vals[i] = values[i]->gen_value()->gen_val();
    return new ConstValue(get_type(),
                          LLVMConstStruct(vals, values.size(), true));
  }
  Type *type = get_type();
  if (is_new) {
    LLVMValueRef ptr = build_malloc(t_type)->gen_val();
    for (size_t i = 0; i < values.size(); i++) {
      auto value = values[i]->gen_value()->gen_val();
      LLVMValueRef set_ptr =
          LLVMBuildStructGEP2(curr_builder, t_type->llvm_type(), ptr, i, UN);
      LLVMBuildStore(curr_builder, value, set_ptr);
    }
    return new ConstValue(t_type->ptr(), ptr);
  } else {
    LLVMValueRef agg = LLVMConstNull(t_type->llvm_type());
    for (size_t i = 0; i < values.size(); i++) {
      auto value = values[i]->gen_value()->gen_val();
      agg = LLVMBuildInsertValue(curr_builder, agg, value, i, UN);
    }
    return new ConstValue(t_type, agg);
  }
}
bool TupleExprAST::is_constant() {
  if (is_new)
    return false;
  for (auto &value : values)
    if (!value->is_constant())
      return false;
  return true;
}