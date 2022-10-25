#pragma once
#include "../types.h"
#include "../utils.h"

class TypeAST {
public:
  virtual ~TypeAST();
  virtual Type *type() = 0;
  LLVMTypeRef llvm_type();
  TypeType type_type();
  virtual bool match(Type *type, uint *generic_count = nullptr) = 0;
  virtual bool is_generic() = 0;
  bool castable_from(Type *type);
  virtual bool eq(TypeAST *other) = 0;
  bool operator==(TypeAST *other);
  bool neq(TypeAST *other);
  virtual std::string stringify();
  PointerType *ptr();
};

struct Generic {
  std::vector<std::string> params;
  TypeAST *ast;
  Generic(std::vector<std::string> params, TypeAST *ast);
  Type *generate(std::vector<Type *> args);
  bool match(Type *type, uint *g);
};

class AbsoluteTypeAST : public TypeAST {
public:
  Type *typ;
  AbsoluteTypeAST(Type *typ);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

inline static auto type_ast(auto t) { return new AbsoluteTypeAST(t); }

class UnaryTypeAST : public TypeAST {
public:
  int opc;
  TypeAST *operand;
  UnaryTypeAST(int opc, TypeAST *operand);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
  std::string stringify();
};

class FunctionTypeAST : public TypeAST {
public:
  TypeAST *return_type;
  std::vector<TypeAST *> args;
  FuncFlags flags;

  FunctionTypeAST(TypeAST *return_type, std::vector<TypeAST *> args,
                  FuncFlags flags);
  FunctionType *func_type();
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

class ArrayTypeAST : public TypeAST {
public:
  TypeAST *elem;
  unsigned int count;
  ArrayTypeAST(TypeAST *elem, unsigned int count);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

class GenericArrayTypeAST : public TypeAST {
public:
  TypeAST *elem;
  std::string count_name;
  GenericArrayTypeAST(TypeAST *elem, std::string count_name);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *generic_count);
  bool is_generic();
  std::string stringify();
};

class StructTypeAST : public TypeAST {
public:
  std::vector<std::pair<std::string, TypeAST *>> members;
  StructTypeAST(std::vector<std::pair<std::string, TypeAST *>> members);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};
class NamedStructTypeAST : public StructTypeAST {
public:
  std::string name;
  NamedStructTypeAST(std::string name,
                     std::vector<std::pair<std::string, TypeAST *>> members);
  Type *type();
};

class TupleTypeAST : public TypeAST {
public:
  std::vector<TypeAST *> types;
  TupleTypeAST(std::vector<TypeAST *> types);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

#include "scope.h"
class NamedTypeAST : public TypeAST {
public:
  Identifier name;
  NamedTypeAST(Identifier name);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

class GenericAccessAST : public TypeAST {
public:
  std::string name;
  std::vector<TypeAST *> params;
  GenericAccessAST(std::string name, std::vector<TypeAST *> params);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
  std::string stringify();
};

class ExprAST;
class TypeofAST : public TypeAST {
public:
  ExprAST *expr;
  TypeofAST(ExprAST *expr);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *g);
  bool is_generic();
};

class UnionTypeAST : public TypeAST {
  TypeAST *picked;

public:
  std::vector<TypeAST *> choices;
  UnionTypeAST(std::vector<TypeAST *> choices);
  // depends on match being called before type
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *generic_count);
  bool is_generic();
  std::string stringify();
};

class GenericTypeAST : public TypeAST {
public:
  std::string name;
  GenericTypeAST(std::string name);
  Type *type();
  bool eq(TypeAST *other);
  bool match(Type *type, uint *generic_count);
  bool is_generic();
  std::string stringify();
};