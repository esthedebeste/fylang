#pragma once
#include "../asts.h"

/// BoolExprAST - Expression class for boolean literals (true or false).
class BoolExprAST : public ExprAST {
  bool value;

public:
  BoolExprAST(bool value);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};