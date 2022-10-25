#pragma once
#include "../asts.h"
#include "../types.h"

class CastExprAST : public ExprAST {
public:
  ExprAST *value;
  TypeAST *to;
  CastExprAST(ExprAST *value, TypeAST *to);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};