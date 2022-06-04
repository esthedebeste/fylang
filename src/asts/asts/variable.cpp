#include "../asts.h"

VariableExprAST::VariableExprAST(std::string name) : name(name) {}
Type *VariableExprAST::get_type() {
  if (auto var = curr_scope->get_named_variable(name))
    return var->get_type();
  else if (curr_named_functions.count(name))
    return curr_named_functions[name]->get_type();
  else
    error("Variable '" + name + "' doesn't exist.");
}
Value *VariableExprAST::gen_value() {
  if (auto var = curr_scope->get_named_variable(name))
    return var;
  else if (curr_named_functions.count(name))
    return curr_named_functions[name]->gen_ptr();
  else
    error("Variable '" + name + "' doesn't exist.");
}