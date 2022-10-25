#pragma once
#include "../asts.h"
#include <vector>

/// ArrayExprAST - Expression class for an array of values.
class ArrayExprAST : public ExprAST {
public:
  std::vector<ExprAST *> elements;
  ArrayExprAST(std::vector<ExprAST *> elements);
  Type *get_type();
  Value *gen_value();
};