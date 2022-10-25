#pragma once
#include "../asts.h"

ConstValue *null_value(Type *type);
ConstValue *null_value();
/// NullExprAST - null
class NullExprAST : public ExprAST {
public:
  Type *type;
  NullExprAST(Type *type);
  NullExprAST();
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};
