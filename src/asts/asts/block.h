#pragma once
#include "../asts.h"
#include <vector>

class BlockExprAST : public ExprAST {

public:
  std::vector<ExprAST *> exprs;
  BlockExprAST(std::vector<ExprAST *> exprs);
  Type *get_type();
  Value *gen_value();
};