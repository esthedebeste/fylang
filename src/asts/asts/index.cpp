#include "index.h"

IndexExprAST::IndexExprAST(ExprAST *value, ExprAST *index)
    : value(value), index(index) {}
Type *IndexExprAST::get_type() {
  Type *base_type = value->get_type();
  if (PointerType *p_type = dynamic_cast<PointerType *>(base_type))
    return p_type->get_points_to();
  else if (ArrayType *arr_type = dynamic_cast<ArrayType *>(base_type))
    return arr_type->get_elem_type();
  else
    error("Invalid index, type not arrayish.\n"
          "Expected: array | pointer \nGot: " +
          base_type->stringify());
}

Value *IndexExprAST::gen_value() {
  Type *type = get_type();
  Value *val = value->gen_value();
  LLVMValueRef index_v = index->gen_value()->gen_val();
  Type *base_type = value->get_type();
  if (PointerType *p_type = dynamic_cast<PointerType *>(base_type)) {
    return new BasicLoadValue(
        type, LLVMBuildGEP2(curr_builder, p_type->get_points_to()->llvm_type(),
                            val->gen_val(), &index_v, 1, UN));
  } else if (ArrayType *arr_type = dynamic_cast<ArrayType *>(base_type)) {
    if (val->has_ptr()) {
      LLVMValueRef index[2] = {LLVMConstNull(NumType(false).llvm_type()),
                               index_v};
      return new BasicLoadValue(
          type, LLVMBuildGEP2(curr_builder, arr_type->llvm_type(),
                              val->gen_ptr(), index, 2, UN));
    } else if (LLVMIsAConstantInt(index_v)) {
      return new ConstValue(
          type, LLVMBuildExtractValue(curr_builder, val->gen_val(),
                                      LLVMConstIntGetZExtValue(index_v), UN));
    } else
      error("Can't index an array that doesn't have a pointer");
  }
  error("Invalid index, type not arrayish.\n"
        "Expected: array | pointer \nGot: " +
        base_type->stringify());
}

NumAccessExprAST::NumAccessExprAST(unsigned int index, ExprAST *source)
    : index(index), source(source) {}

Type *NumAccessExprAST::get_type() {
  Type *st = source->get_type();
  is_ptr = st->type_type() == TypeType::Pointer;
  if (is_ptr)
    st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
  source_type = dynamic_cast<TupleType *>(st);
  return source_type->get_elem_type(index);
}

Value *NumAccessExprAST::gen_value() {
  Type *type = get_type();
  Value *src = source->gen_value();
  if (!is_ptr && !src->has_ptr())
    return new ConstValue(
        type, LLVMBuildExtractValue(curr_builder, src->gen_val(), index, UN));
  // If src is a struct-pointer (*String) then access on the value, if src
  // is a struct-value (String) then access on the pointer to where it's
  // stored.
  LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
  return new BasicLoadValue(type, LLVMBuildStructGEP2(curr_builder,
                                                      source_type->llvm_type(),
                                                      struct_ptr, index, UN));
}

PropAccessExprAST::PropAccessExprAST(std::string key, ExprAST *source)
    : key(key), source(source) {}

Type *PropAccessExprAST::get_type() {
  Type *st = source->get_type();
  if (st->type_type() == TypeType::Pointer) {
    st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
    is_ptr = true;
  } else
    is_ptr = false;
  StructType *struct_t = dynamic_cast<StructType *>(st);
  if (!struct_t)
    error("Invalid property access for key '" + key + "', " + st->stringify() +
          " is not a struct.");
  index = struct_t->get_index(key);
  Type *type = struct_t->get_elem_type(index);
  source_type = struct_t;
  return type;
}

Value *PropAccessExprAST::gen_value() {
  Type *type = get_type();
  Value *src = source->gen_value();
  if (!is_ptr && !src->has_ptr())
    return new ConstValue(
        type, LLVMBuildExtractValue(curr_builder, src->gen_val(), index, UN));
  // If src is a struct-pointer (*String) then access on the value, if src
  // is a struct-value (String) then access on the pointer to where it's
  // stored.
  LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
  return new BasicLoadValue(type, LLVMBuildStructGEP2(curr_builder,
                                                      source_type->llvm_type(),
                                                      struct_ptr, index, UN));
}