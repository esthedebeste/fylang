#pragma once
#include "../asts.h"

/// OrExprAST - Expression class for a short-circuiting or
class OrExprAST : public ExprAST {
public:
  ExprAST *left, *right;
  OrExprAST(ExprAST *left, ExprAST *right);
  Type *get_type();
  Value *gen_value();
};

/// AndExprAST - Expression class for a short-circuiting and
class AndExprAST : public OrExprAST {
public:
  using OrExprAST::OrExprAST;
  Value *gen_value();
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
  ExprAST *cond, *then, *elze;
  bool null_else;
  Type *type;
  void init();
  IfExprAST(ExprAST *cond, ExprAST *then,
            // elze because else cant be a variable name lol
            ExprAST *elze);

  Type *get_type();

  Value *gen_value();
};