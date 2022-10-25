#pragma once
#include "../asts.h"

/// IndexExprAST - Expression class for accessing indexes (a[0]).
class IndexExprAST : public ExprAST {
  ExprAST *value;
  ExprAST *index;

public:
  IndexExprAST(ExprAST *value, ExprAST *index);
  Type *get_type();
  Value *gen_value();
};

/// NumAccessExprAST - Expression class for accessing indexes on Tuples (a.0).
class NumAccessExprAST : public ExprAST {
  bool is_ptr;
  TupleType *source_type;

public:
  unsigned int index;
  ExprAST *source;
  NumAccessExprAST(unsigned int index, ExprAST *source);
  Type *get_type();
  Value *gen_value();
};

std::tuple<Type *, StructType *, size_t, bool> get_prop_type(Type *type,
                                                             std::string key);
Value *get_prop_value(Value *value, std::string key);
/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  bool is_ptr;
  TupleType *source_type;
  size_t index;

public:
  std::string key;
  ExprAST *source;
  PropAccessExprAST(std::string key, ExprAST *source);

  Type *get_type();
  Value *gen_value();
};