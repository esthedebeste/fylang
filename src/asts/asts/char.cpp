#include "char.h"

CharExprAST::CharExprAST(char data)
    : NumberExprAST(data, NumType(8, false, false)) {}