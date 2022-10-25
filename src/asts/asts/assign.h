#pragma once
#include "../asts.h"

class AssignExprAST : public ExprAST {
  ExprAST *LHS, *RHS;

public:
  AssignExprAST(ExprAST *LHS, ExprAST *RHS);
  Type *get_type();
  Value *gen_value();
};