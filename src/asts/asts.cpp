#include "asts.h"

ExprAST::~ExprAST() {}
bool ExprAST::is_constant() { return false; }