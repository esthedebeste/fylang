#pragma once
#include "../types.h"
#include "../values.h"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST();
  virtual Type *get_type() = 0;
  virtual Value *gen_value() = 0;
  virtual bool is_constant();
};