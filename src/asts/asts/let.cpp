#include "let.h"
#include "cast.h"
#include "utils.h"

LetExprAST::LetExprAST(std::string id, TypeAST *type, ExprAST *value,
                       bool constant)
    : id(id), type(type), value(value), constant(constant),
      untyped(type == nullptr) {}
Type *LetExprAST::get_type() {
  Type *type;
  if (untyped) {
    if (value)
      type = value->get_type();
    else
      error("Untyped valueless variable " + id);
  } else
    type = this->type->type();
  curr_scope->declare_variable(id, type);
  return type;
}
LLVMValueRef LetExprAST::gen_toplevel() {
  Type *type = get_type();
  std::string name = curr_scope->get_prefix() + id;
  auto ptr = LLVMAddGlobal(curr_module, type->llvm_type(), name.c_str());
  Value *var_value = new BasicLoadValue(type, ptr);
  LLVMSetInitializer(ptr, LLVMConstNull(type->llvm_type()));
  if (value) {
    if (value->is_constant()) {
      LLVMValueRef val = value->gen_value()->cast_to(type)->gen_val();
      LLVMSetInitializer(ptr, val);
      if (constant)
        var_value = new ConstValueWithPtr(type, ptr, val);
    } else
      add_store_before_main(ptr, new CastExprAST(value, type_ast(type)));
  }
  curr_scope->set_variable(id, var_value);
  if (constant)
    LLVMSetGlobalConstant(ptr, true);
  return ptr;
}
Value *LetExprAST::gen_value() {
  Type *type = get_type();
  if (constant) {
    if (value) {
      auto val =
          new ConstValue(type, value->gen_value()->cast_to(type)->gen_val());
      curr_scope->set_variable(id, val);
      return val;
    } else
      error("Constant variables need an initialization value");
  }
  LLVMValueRef ptr =
      LLVMBuildAlloca(curr_builder, type->llvm_type(), id.c_str());
  LLVMSetValueName2(ptr, id.c_str(), id.size());
  if (value) {
    LLVMValueRef llvm_val = value->gen_value()->cast_to(type)->gen_val();
    LLVMBuildStore(curr_builder, llvm_val, ptr);
  }
  BasicLoadValue *val = new BasicLoadValue(type, ptr);
  curr_scope->set_variable(id, val);
  return val;
}
LLVMValueRef LetExprAST::gen_declare() {
  Type *type = get_type();
  LLVMValueRef global =
      LLVMAddGlobal(curr_module, type->llvm_type(), id.c_str());
  curr_scope->set_variable(id, new BasicLoadValue(type, global));
  return global;
}