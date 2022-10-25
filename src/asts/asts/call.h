#pragma once
#include "../asts.h"
#include "../functions.h"
#include "../scope.h"
#include <vector>

/// ValueCallExprAST - For calling a value (often a function pointer)
class ValueCallExprAST : public ExprAST {
  ExprAST *called;
  std::vector<ExprAST *> args;
  bool is_ptr;
  FunctionType *get_func_type();

public:
  ValueCallExprAST(ExprAST *called, std::vector<ExprAST *> args);

  Type *get_type();
  Value *gen_value();
};

class NameCallExprAST : public ExprAST {
public:
  Identifier name;
  std::vector<ExprAST *> args;
  NameCallExprAST(Identifier name, std::vector<ExprAST *> args);
  Type *get_type();
  Value *gen_value();
};

struct ExtAndPtr {
  MethodAST *extension;
  bool is_ptr;
};
/// MethodCallExprAST - Expression class for calling methods (a.len()).
class MethodCallExprAST : public ExprAST {
  ExtAndPtr get_extension();

public:
  std::string name;
  ExprAST *source;
  std::vector<ExprAST *> args;
  MethodCallExprAST(std::string name, ExprAST *source,
                    std::vector<ExprAST *> args);

  Type *get_type();
  Value *gen_value();
};