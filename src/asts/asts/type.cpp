#include "../asts.h"

TypeAssertExprAST::TypeAssertExprAST(TypeAST *a, TypeAST *b) : a(a), b(b) {}
Type *TypeAssertExprAST::get_type() {
  if (a->type()->neq(b->type()))
    error("Type mismatch in Type assertion, " + a->stringify() +
          " != " + b->stringify());
  return new NullType();
}
Value *TypeAssertExprAST::gen_value() { return null_value(new NullType()); }

TypeDumpExprAST::TypeDumpExprAST(TypeAST *type) : type(type) {}
Type *TypeDumpExprAST::get_type() {
  std::cout << "[DUMP] Dumped type: " << type->stringify();
  return new NullType();
}
Value *TypeDumpExprAST::gen_value() { return null_value(new NullType()); }

ExprAST *TypeIfExprAST::pick() { return b->match(a->type()) ? then : elze; }

TypeIfExprAST::TypeIfExprAST(TypeAST *a, TypeAST *b, ExprAST *then,
                             // elze because else cant be a variable name lol
                             ExprAST *elze)
    : a(a), b(b), then(then), elze(elze) {}

Type *TypeIfExprAST::get_type() { return pick()->get_type(); }
Value *TypeIfExprAST::gen_value() { return pick()->gen_value(); }
bool TypeIfExprAST::is_constant() { return pick()->is_constant(); }