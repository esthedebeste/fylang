#pragma once
#include "../asts.h"
#include "../types.h"
#include <vector>

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {

public:
  TypeAST *s_type;
  std::vector<std::pair<std::string, ExprAST *>> fields;
  bool is_new;
  NewExprAST(TypeAST *s_type,
             std::vector<std::pair<std::string, ExprAST *>> fields,
             bool is_new);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};