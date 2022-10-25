#pragma once
#include "number.h"

/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public NumberExprAST {
public:
  CharExprAST(char data);
};