#pragma once
#include "../asts.h"
#include "../scope.h"

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {

public:
  Identifier name;
  VariableExprAST(Identifier name);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};