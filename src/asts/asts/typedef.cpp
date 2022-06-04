#include "../asts.h"

TypeDefAST::~TypeDefAST() {}

AbsoluteTypeDefAST::AbsoluteTypeDefAST(std::string name, TypeAST *type)
    : name(name), type(type) {}
void AbsoluteTypeDefAST::gen_toplevel() {
  curr_named_types[name] = type->type();
}

GenericTypeDefAST::GenericTypeDefAST(std::string name,
                                     std::vector<std::string> params,
                                     TypeAST *type)
    : name(name), params(params), type(type) {}
void GenericTypeDefAST::gen_toplevel() {
  curr_named_generics[name] = new Generic(params, type);
}
