#include "../asts.h"

Scope::Scope(Scope *parent_scope) : parent_scope(parent_scope) {}
Value *Scope::get_named_variable(std::string name) {
  if (named_variables.count(name))
    return named_variables[name];
  if (parent_scope)
    return parent_scope->get_named_variable(name);
  return nullptr;
}
void Scope::declare_variable(std::string name, Type *type) {
  named_variables[name] = new ConstValue(type, nullptr);
}
void Scope::set_variable(std::string name, Value *value) {
  named_variables[name] = value;
}

Scope *global_scope = new Scope(nullptr);
Scope *curr_scope = global_scope;
Scope *push_scope() { return curr_scope = new Scope(curr_scope); }
Scope *pop_scope() {
  for (auto &[name, value] : curr_scope->named_variables) {
    Type *type = value->get_type();
    FunctionAST *destructor = type->get_destructor();
    if (!destructor)
      continue;
    LLVMValueRef llvm_val = value->gen_val();
    if (!llvm_val) // type phase
      continue;
    ConstValue val = ConstValue(type, llvm_val);
    destructor->gen_call({&val});
  }
  return curr_scope = curr_scope->parent_scope;
}