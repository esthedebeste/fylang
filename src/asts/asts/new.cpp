#include "new.h"
#include "utils.h"

NewExprAST::NewExprAST(TypeAST *s_type,
                       std::vector<std::pair<std::string, ExprAST *>> fields,
                       bool is_new)
    : s_type(s_type), fields(fields), is_new(is_new) {}
Type *NewExprAST::get_type() {
  return is_new ? s_type->type()->ptr() : s_type->type();
}

Value *NewExprAST::gen_value() {
  StructType *st = dynamic_cast<StructType *>(s_type->type());
  if (!st)
    error("Cannot create instance of non-struct type " +
          s_type->type()->stringify());
  LLVMValueRef agg = LLVMConstNull(st->llvm_type());
  for (size_t i = 0; i < fields.size(); i++) {
    auto &[key, value] = fields[i];
    size_t index = key == "" ? i : st->get_index(key);
    agg = LLVMBuildInsertValue(
        curr_builder, agg,
        value->gen_value()->cast_to(st->get_elem_type(index))->gen_val(), index,
        key.c_str());
  }
  if (is_new) {
    LLVMValueRef ptr = build_malloc(st)->gen_val();
    LLVMBuildStore(curr_builder, agg, ptr);
    return new ConstValue(s_type->type()->ptr(), ptr);
  } else {
    return new ConstValue(s_type->type(), agg);
  }
}
bool NewExprAST::is_constant() {
  if (is_new)
    return false;
  for (auto &[key, value] : fields)
    if (!value->is_constant())
      return false;
  return true;
}