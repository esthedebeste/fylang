#pragma once
#include "../asts.h"
#include "../types.h"
#include <string>

class TypeDefAST {
public:
  virtual ~TypeDefAST();
  virtual void gen_toplevel() = 0;
};

class AbsoluteTypeDefAST : public TypeDefAST {
  std::string name;
  TypeAST *type;

public:
  AbsoluteTypeDefAST(std::string name, TypeAST *type);
  void gen_toplevel();
};

class GenericTypeDefAST : public TypeDefAST {
public:
  std::string name;
  std::vector<std::string> params;
  TypeAST *type;
  GenericTypeDefAST(std::string name, std::vector<std::string> params,
                    TypeAST *type);
  void gen_toplevel();
};