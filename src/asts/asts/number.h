#pragma once
#include "../asts.h"

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  union {
    long double floating;
    unsigned long long integer;
  } value;

public:
  NumType type;
  NumberExprAST(std::string val, char type_char, bool has_dot,
                unsigned int base);
  NumberExprAST(unsigned long long val, char type_char);
  NumberExprAST(unsigned long long val, NumType type);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};