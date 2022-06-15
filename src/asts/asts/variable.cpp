#include "../asts.h"

VariableExprAST::VariableExprAST(Identifier name) : name(name) {}
Type *VariableExprAST::get_type() {
  if (auto var = get_variable(name))
    return var->get_type();
  else if (auto func = get_function(name))
    return func->get_type();
  else
    error("Variable '" + name.to_str() + "' doesn't exist.");
}
Value *VariableExprAST::gen_value() {
  if (auto var = get_variable(name))
    return var;
  else if (auto func = get_function(name))
    return func->gen_ptr();
  else
    error("Variable '" + name.to_str() + "' doesn't exist.");
}
bool VariableExprAST::is_constant() {
  if (auto var = get_variable(name))
    return var->is_constant();
  else if (auto func = get_function(name))
    return true;
  else
    error("Variable '" + name.to_str() + "' doesn't exist.");
}