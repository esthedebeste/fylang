#pragma once
#include "../asts.h"
#include "../types.h"

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST {

public:
  std::string id;
  bool untyped;
  TypeAST *type;
  ExprAST *value;
  bool constant;
  LetExprAST(std::string id, TypeAST *type, ExprAST *value, bool constant);
  LLVMValueRef gen_toplevel();
  Value *gen_value();
  Type *get_type();
  LLVMValueRef gen_declare();
};