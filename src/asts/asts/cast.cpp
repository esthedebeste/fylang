#include "cast.h"

CastExprAST::CastExprAST(ExprAST *value, TypeAST *to) : value(value), to(to) {}
Type *CastExprAST::get_type() { return to->type(); }
Value *CastExprAST::gen_value() {
  return value->gen_value()->cast_to(to->type());
}
bool CastExprAST::is_constant() { return value->is_constant(); }