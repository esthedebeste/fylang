#pragma once
#include "../asts.h"

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  int op;
  ExprAST *operand;

public:
  UnaryExprAST(int op, ExprAST *operand);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};