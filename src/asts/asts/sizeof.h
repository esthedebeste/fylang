#pragma once
#include "../asts.h"
#include "../types.h"

/// SizeofExprAST - Expression class to get the byte size of a type
class SizeofExprAST : public ExprAST {
public:
  TypeAST *type;
  SizeofExprAST(TypeAST *type);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};