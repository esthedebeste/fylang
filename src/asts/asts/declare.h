#pragma once
#include "../asts.h"
#include "../functions.h"
#include "let.h"

/// DeclareExprAST - Expression class for defining a declare.
class DeclareExprAST {
  LetExprAST *let = nullptr;
  FunctionAST *func = nullptr;

public:
  DeclareExprAST(LetExprAST *let);
  DeclareExprAST(FunctionAST *func);
  LLVMValueRef gen_toplevel();
};