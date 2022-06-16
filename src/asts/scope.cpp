#include "scope.h"
#include "../values.h"
#include "functions.h"

Scope::Scope(Scope *parent_scope, std::string name)
    : parent_scope(parent_scope), name(name) {}

static std::vector<std::string> full_path(Scope *curr) {
  std::vector<std::string> ret;
  while (curr) {
    if (curr->name != "")
      ret.push_back(curr->name);
    curr = curr->parent_scope;
  }
  return ret;
}
std::string Scope::get_prefix() {
  auto path = full_path(this);
  std::stringstream ret;
  for (auto it = path.rbegin(); it != path.rend(); it++)
    ret << *it << "::";
  return ret.str();
}

#define _get(type, sing)                                                       \
  if (named_##type.count(name))                                                \
    return named_##type[name];                                                 \
  if (parent_scope)                                                            \
    return parent_scope->get_##sing(name);                                     \
  return nullptr

Value *Scope::get_variable(std::string name) { _get(variables, variable); }
FunctionAST *Scope::get_function(std::string name) {
  _get(functions, function);
}
Type *Scope::get_type(std::string name) { _get(types, type); }
Scope *Scope::get_scope(std::string name) { _get(scopes, scope); }
Generic *Scope::get_generic(std::string name) { _get(generics, generic); }
void Scope::declare_variable(std::string name, Type *type) {
  named_variables[name] = new ConstValue(type, nullptr);
}
void Scope::set_variable(std::string name, Value *value) {
  named_variables[name] = value;
}
void Scope::set_function(std::string name, FunctionAST *function) {
  named_functions[name] = function;
}
void Scope::set_type(std::string name, Type *type) { named_types[name] = type; }
void Scope::set_scope(std::string name, Scope *scope) {
  named_scopes[name] = scope;
}
void Scope::set_generic(std::string name, Generic *generic) {
  named_generics[name] = generic;
}

#define _resolve(type, sing)                                                   \
  if (!id.has_spaces())                                                        \
    return scope->get_##sing(id.name);                                         \
  scope = scope->get_scope(id.spaces[0]); /* allow going up and down the scope \
                                             tree for the first scope */       \
                                                                               \
  if (!scope)                                                                  \
    error("First scope for '" + id.to_str() + "' doesn't exist.");             \
  for (size_t i = 1; i < id.spaces.size(); i++) {                              \
    scope = scope->named_scopes[id.spaces[i]];                                 \
    if (!scope)                                                                \
      error("Scope for '" + id.to_str() + "' doesn't exist.");                 \
  }                                                                            \
  return scope->get_##sing(id.name)

Value *get_variable(Identifier id, Scope *scope) {
  _resolve(variables, variable);
}
FunctionAST *get_function(Identifier id, Scope *scope) {
  _resolve(functions, function);
}
Type *get_type(Identifier id, Scope *scope) { _resolve(types, type); }
Generic *get_generic(Identifier id, Scope *scope) {
  _resolve(generics, generic);
}

Scope global_scope(nullptr);
Scope *curr_scope = &global_scope;
Scope *push_scope() { return curr_scope = new Scope(curr_scope); }
Scope *push_space(std::string name) {
  auto space = new Scope(curr_scope, name);
  curr_scope->set_scope(name, space);
  return curr_scope = space;
}
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
Scope *pop_space() { return curr_scope = curr_scope->parent_scope; }