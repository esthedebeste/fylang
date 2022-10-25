#pragma once
#include "../asts.h"
#include <vector>

class TupleExprAST : public ExprAST {
  std::vector<ExprAST *> values;
  TupleType *t_type;

public:
  bool is_new = false;
  TupleExprAST(std::vector<ExprAST *> values);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};