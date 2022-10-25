#pragma once
#include "../asts.h"
#include "../types.h"

class TypeAssertExprAST : public ExprAST {
public:
  TypeAST *a;
  TypeAST *b;
  TypeAssertExprAST(TypeAST *a, TypeAST *b);
  Type *get_type();
  Value *gen_value();
};

class TypeDumpExprAST : public ExprAST {
public:
  TypeAST *type;
  TypeDumpExprAST(TypeAST *type);
  Type *get_type();
  Value *gen_value();
};

/// TypeIfExprAST - Expression class for ifs based on type.
class TypeIfExprAST : public ExprAST {
  ExprAST *pick();

public:
  ExprAST *then, *elze;
  TypeAST *a, *b;
  TypeIfExprAST(TypeAST *a, TypeAST *b, ExprAST *then,
                // elze because else cant be a variable name lol
                ExprAST *elze);

  Type *get_type();
  Value *gen_value();
  bool is_constant();
};
