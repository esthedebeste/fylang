#pragma once
#include "../utils.h"

struct Identifier {
  std::vector<std::string> spaces;
  std::string name;
  Identifier(std::vector<std::string> spaces, std::string name);
  Identifier(std::string name);
  bool has_spaces();
  std::string to_str();
};
template <> struct std::hash<Identifier> {
  size_t operator()(Identifier t) const noexcept {
    size_t res;
    for (auto c : t.spaces)
      res = (res << 4) + std::hash<std::string>()(c);
    res = (res << 4) + std::hash<std::string>()(t.name);
    return res;
  }
};
bool operator==(const Identifier &lhs, const Identifier &rhs);

class Value;
class FunctionAST;
class Type;
class Generic;
struct Scope {
  std::unordered_map<std::string, Value *> named_variables;
  std::unordered_map<std::string, FunctionAST *> named_functions;
  std::unordered_map<std::string, Type *> named_types;
  std::unordered_map<std::string, Scope *> named_scopes;
  std::unordered_map<std::string, Generic *> named_generics;
  std::string name;
  Scope *parent_scope;
  Scope(Scope *parent_scope = nullptr, std::string name = "");
  Value *get_variable(std::string name);
  FunctionAST *get_function(std::string name);
  Type *get_type(std::string name);
  Scope *get_scope(std::string name);
  Generic *get_generic(std::string name);
  void declare_variable(std::string name, Type *type);
  void set_variable(std::string name, Value *value);
  void set_function(std::string name, FunctionAST *value);
  void set_type(std::string name, Type *value);
  void set_scope(std::string name, Scope *value);
  void set_generic(std::string name, Generic *value);
  std::string get_prefix();
};
extern Scope *curr_scope;

Value *get_variable(Identifier id, Scope *base = curr_scope);
FunctionAST *get_function(Identifier id, Scope *base = curr_scope);
Type *get_type(Identifier id, Scope *base = curr_scope);
Generic *get_generic(Identifier id, Scope *base = curr_scope);

Scope *push_scope();
Scope *push_space(std::string name);
Scope *pop_scope();
Scope *pop_space();