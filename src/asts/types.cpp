#pragma once
#include "../types.cpp"
#include "../utils.cpp"
#include "asts.cpp"

static std::unordered_map<std::string, Type *> curr_named_types;
class TypeAST {
  Type *_type = nullptr;
  virtual Type *gen_type() = 0;

public:
  virtual ~TypeAST() {}
  Type *type() {
    if (!_type)
      return _type = gen_type();
    return _type;
  }

  LLVMTypeRef llvm_type() { return type()->llvm_type(); }
  TypeType type_type() { return type()->type_type(); }
  virtual bool eq(TypeAST *other) = 0;
  bool operator==(TypeAST *other) { return eq(other); }
  bool neq(TypeAST *other) { return !eq(other); };
  std::string stringify() { return type()->stringify(); }
  PointerType *ptr() { return type()->ptr(); }
};

class AbsoluteTypeAST : public TypeAST {
public:
  Type *type;
  AbsoluteTypeAST(Type *type) : type(type) {}
  Type *gen_type() { return type; }
  bool eq(TypeAST *other) { return type->eq(other->type()); }
};
static AbsoluteTypeAST *type_ast(Type *t) { return new AbsoluteTypeAST(t); }

class UnaryTypeAST : public TypeAST {
public:
  int opc;
  TypeAST *type;
  UnaryTypeAST(int opc, TypeAST *type) : opc(opc), type(type) {}
  Type *gen_type() {
    Type *operand = type->type();
    switch (opc) {
    case '*':
      return operand->ptr();
    case '&':
      if (PointerType *ptr = dynamic_cast<PointerType *>(operand))
        return ptr->get_points_to();
      else
        error("use of & type operator without pointer on right-hand side");
    case T_UNSIGNED:
      if (NumType *num = dynamic_cast<NumType *>(operand))
        if (!num->is_floating)
          return new NumType(num->bits, false, false);
      error("use of 'unsigned' type operator with non-int on right-hand side");
    case T_SIGNED:
      if (NumType *num = dynamic_cast<NumType *>(operand))
        return new NumType(num->bits, num->is_floating, true);
      else
        error(
            "use of 'signed' type operator with non-number on right-hand side");
    default:
      error("unimplemented unary op " + token_to_str(opc));
    }
  }
  bool eq(TypeAST *other) {
    if (UnaryTypeAST *u = dynamic_cast<UnaryTypeAST *>(other))
      return opc == u->opc && type->eq(u->type);
    else
      return TypeAST::type()->eq(other->type());
    return false;
  }
};

class FunctionTypeAST : public TypeAST {
public:
  TypeAST *return_type;
  std::vector<TypeAST *> args;
  bool vararg;

  FunctionTypeAST(TypeAST *return_type, std::vector<TypeAST *> args,
                  bool vararg)
      : return_type(return_type), args(args), vararg(vararg) {}
  Type *gen_type() {
    std::vector<Type *> arg_types;
    for (auto arg : args)
      arg_types.push_back(arg->type());
    return new FunctionType(return_type->type(), arg_types, vararg);
  }
  bool eq(TypeAST *other) {
    FunctionTypeAST *f = dynamic_cast<FunctionTypeAST *>(other);
    if (f == nullptr || args.size() != f->args.size() || vararg != f->vararg)
      return false;
    for (unsigned int i = 0; i < args.size(); i++)
      if (!args[i]->eq(f->args[i]))
        return false;
    return return_type->eq(f->return_type);
  }
};

class ArrayTypeAST : public TypeAST {
public:
  TypeAST *type;
  int size;
  ArrayTypeAST(TypeAST *type, int size) : type(type), size(size) {}
  Type *gen_type() { return new ArrayType(type->type(), size); }
  bool eq(TypeAST *other) {
    ArrayTypeAST *a = dynamic_cast<ArrayTypeAST *>(other);
    if (!a)
      return false;
    return type->eq(a->type) && size == a->size;
  }
};

class StructTypeAST : public TypeAST {
public:
  std::string name;
  std::vector<std::pair<std::string, TypeAST *>> members;
  StructTypeAST(std::string name,
                std::vector<std::pair<std::string, TypeAST *>> members)
      : name(name), members(members) {}
  Type *gen_type() {
    std::vector<std::pair<std::string, Type *>> types;
    for (auto &m : members)
      types.push_back(std::make_pair(m.first, m.second->type()));
    return new StructType(name, types);
  }
  bool eq(TypeAST *other) {
    StructTypeAST *s = dynamic_cast<StructTypeAST *>(other);
    if (!s)
      return false;
    if (members.size() != s->members.size())
      return false;
    for (size_t i = 0; i < members.size(); i++)
      if (members[i].first != s->members[i].first ||
          members[i].second->neq(s->members[i].second))
        return false;
    return true;
  }
};

class TupleTypeAST : public TypeAST {
public:
  std::vector<TypeAST *> types;
  TupleTypeAST(std::vector<TypeAST *> types) : types(types) {}
  Type *gen_type() {
    std::vector<Type *> fields(types.size());
    for (size_t i = 0; i < types.size(); i++)
      fields[i] = types[i]->type();
    return new TupleType(fields);
  }
  bool eq(TypeAST *other) {
    TupleTypeAST *t = dynamic_cast<TupleTypeAST *>(other);
    if (!t)
      return false;
    if (types.size() != t->types.size())
      return false;
    for (size_t i = 0; i < types.size(); i++)
      if (types[i]->neq(t->types[i]))
        return false;
    return true;
  }
};

class NamedTypeAST : public TypeAST {
public:
  std::string name;
  NamedTypeAST(std::string name) : name(name) {}
  Type *gen_type() { return curr_named_types[name]; }
  bool eq(TypeAST *other) { return type()->eq(other->type()); }
};

class TypeofAST : public TypeAST {
public:
  ExprAST *expr;
  TypeofAST(ExprAST *expr) : expr(expr) {}
  Type *gen_type() { return expr->get_type(); }
  bool eq(TypeAST *other) { return type()->eq(other->type()); }
};

class GenericTypeAST : public TypeAST {
public:
  std::string name;
  GenericTypeAST(std::string name) : name(name) {}
  Type *gen_type() { return nullptr; }
  bool eq(TypeAST *other) {
    curr_named_types[name] = other->type();
    return true;
  }
};