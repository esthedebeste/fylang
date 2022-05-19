#pragma once
#include "../types.cpp"
#include "../utils.cpp"
#include "asts.cpp"

static std::unordered_map<std::string, Type *> curr_named_types;
class TypeAST {
public:
  virtual ~TypeAST() {}
  virtual Type *type() = 0;
  LLVMTypeRef llvm_type() { return type()->llvm_type(); }
  TypeType type_type() { return type()->type_type(); }
  virtual bool match(Type *type, uint *generic_count = nullptr) = 0;
  virtual bool is_generic() = 0;
  bool castable_from(Type *type) {
    if (is_generic())
      return match(type);
    return match(type) || type->castable_to(this->type());
  }
  virtual bool eq(TypeAST *other) = 0;
  bool operator==(TypeAST *other) { return eq(other); }
  bool neq(TypeAST *other) { return !eq(other); };
  virtual std::string stringify() { return type()->stringify(); }
  PointerType *ptr() { return type()->ptr(); }
};

struct Generic {
  std::vector<std::string> params;
  TypeAST *ast;
  Generic(std::vector<std::string> params, TypeAST *ast)
      : params(params), ast(ast) {}
  Type *generate(std::vector<Type *> args) {
    if (args.size() != params.size())
      error("wrong number of arguments to generic type");
    for (size_t i = 0; i < params.size(); i++)
      curr_named_types[params[i]] = args[i];
    Type *type = ast->type();
    return type;
  }
};
static std::unordered_map<std::string, Generic *> curr_named_generics;

class AbsoluteTypeAST : public TypeAST {
public:
  Type *typ;
  AbsoluteTypeAST(Type *typ) : typ(typ) {}
  Type *type() { return typ; }
  bool eq(TypeAST *other) { return typ->eq(other->type()); }
  bool match(Type *type, uint *g) { return typ->eq(type); }
  bool is_generic() { return false; }
};
static TypeAST *type_ast(Type *t) { return new AbsoluteTypeAST(t); }

class UnaryTypeAST : public TypeAST {
public:
  int opc;
  TypeAST *operand;
  UnaryTypeAST(int opc, TypeAST *operand) : opc(opc), operand(operand) {}
  Type *type() {
    Type *operand = this->operand->type();
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
      return opc == u->opc && operand->eq(u->operand);
    else if (AbsoluteTypeAST *a = dynamic_cast<AbsoluteTypeAST *>(other))
      return match(a->typ, nullptr);
    return false;
  }
  bool match(Type *type, uint *g) {
    switch (opc) {
    case '*':
      if (PointerType *ptr = dynamic_cast<PointerType *>(type))
        return operand->match(ptr->get_points_to(), g);
      else
        return false;
    case '&':
      return operand->match(type->ptr(), g);
    case T_UNSIGNED:
      if (NumType *num = dynamic_cast<NumType *>(type))
        return num->is_signed == false &&
               operand->match(new NumType(num->bits, num->is_floating, true),
                              g);
      else
        return false;
    case T_SIGNED:
      if (NumType *num = dynamic_cast<NumType *>(type))
        return num->is_signed &&
               operand->match(new NumType(num->bits, num->is_floating, true),
                              g);
      else
        return false;
    }
    error("unimplemented unary op " + token_to_str(opc));
  }
  bool is_generic() { return operand->is_generic(); }
  std::string stringify() {
    switch (opc) {
    case '*':
    case '&':
      return ((char)opc) + operand->stringify();
    case T_UNSIGNED:
    case T_SIGNED:
      return token_to_str(opc) + " " + operand->stringify();
    default:
      error("unimplemented stringify for unary op " + token_to_str(opc));
    }
  }
};

class FunctionTypeAST : public TypeAST {
public:
  TypeAST *return_type;
  std::vector<TypeAST *> args;
  FuncFlags flags;

  FunctionTypeAST(TypeAST *return_type, std::vector<TypeAST *> args,
                  FuncFlags flags)
      : return_type(return_type), args(args), flags(flags) {}
  FunctionType *func_type() {
    std::vector<Type *> arg_types;
    for (auto arg : args)
      arg_types.push_back(arg->type());
    return new FunctionType(return_type ? return_type->type() : nullptr,
                            arg_types, flags);
  }
  Type *type() { return func_type(); }
  bool eq(TypeAST *other) {
    FunctionTypeAST *f = dynamic_cast<FunctionTypeAST *>(other);
    if (f == nullptr || args.size() != f->args.size() || flags.neq(f->flags))
      return false;
    for (size_t i = 0; i < args.size(); i++)
      if (!args[i]->eq(f->args[i]))
        return false;
    return return_type->eq(f->return_type);
  }
  bool match(Type *type, uint *g) {
    if (FunctionType *f = dynamic_cast<FunctionType *>(type)) {
      if (args.size() != f->arguments.size() || flags.neq(f->flags))
        return false;
      for (size_t i = 0; i < args.size(); i++)
        if (!args[i]->match(f->arguments[i], g))
          return false;
      return return_type->match(f->return_type, g);
    }
    return false;
  }
  bool is_generic() {
    for (auto arg : args)
      if (arg->is_generic())
        return true;
    return return_type == nullptr ? false : return_type->is_generic();
  }
};

class ArrayTypeAST : public TypeAST {
public:
  TypeAST *elem;
  unsigned int count;
  ArrayTypeAST(TypeAST *elem, unsigned int count) : elem(elem), count(count) {}
  Type *type() { return new ArrayType(elem->type(), count); }
  bool eq(TypeAST *other) {
    if (ArrayTypeAST *a = dynamic_cast<ArrayTypeAST *>(other))
      return elem->eq(a->elem) && count == a->count;
    return false;
  }
  bool match(Type *type, uint *g) {
    if (ArrayType *a = dynamic_cast<ArrayType *>(type))
      return this->elem->match(a->elem, g) && count == a->count;
    return false;
  }
  bool is_generic() { return elem->is_generic(); }
};

class GenericArrayTypeAST : public TypeAST {
public:
  TypeAST *elem;
  std::string count_name;
  GenericArrayTypeAST(TypeAST *elem, std::string count_name)
      : elem(elem), count_name(count_name) {}
  Type *type() {
    IntValue *count =
        dynamic_cast<IntValue *>(curr_scope->get_named_variable(count_name));
    if (count == nullptr)
      error("Generic array not initialized properly, " << count_name
                                                       << " is undefined.");
    return new ArrayType(elem->type(), count->val);
  }
  bool eq(TypeAST *other) {
    if (GenericArrayTypeAST *a = dynamic_cast<GenericArrayTypeAST *>(other))
      return elem->eq(a->elem) && count_name == a->count_name;
    return false;
  }
  bool match(Type *type, uint *generic_count) {
    if (ArrayType *a = dynamic_cast<ArrayType *>(type)) {
      if (this->elem->match(a->elem, generic_count)) {
        curr_scope->set_variable(count_name,
                                 new IntValue(NumType(false), a->count));
        if (generic_count)
          (*generic_count) += 1;
        return true;
      }
    }
    return false;
  }
  bool is_generic() { return true; }
  std::string stringify() {
    return elem->stringify() + "[generic " + count_name + "]";
  }
};

class StructTypeAST : public TypeAST {
public:
  std::vector<std::pair<std::string, TypeAST *>> members;
  StructTypeAST(std::vector<std::pair<std::string, TypeAST *>> members)
      : members(members) {}
  Type *type() {
    std::vector<std::pair<std::string, Type *>> types;
    for (auto &m : members)
      types.push_back(std::make_pair(m.first, m.second->type()));
    return new StructType(types);
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
  bool match(Type *type, uint *g) {
    if (StructType *s = dynamic_cast<StructType *>(type)) {
      if (members.size() != s->fields.size())
        return false;
      for (size_t i = 0; i < members.size(); i++)
        if (members[i].first != s->fields[i].first ||
            !members[i].second->match(s->fields[i].second, g))
          return false;
      return true;
    }
    return false;
  }
  bool is_generic() {
    for (auto &m : members)
      if (m.second->is_generic())
        return true;
    return false;
  }
};
class NamedStructTypeAST : public StructTypeAST {
public:
  std::string name;
  NamedStructTypeAST(std::string name,
                     std::vector<std::pair<std::string, TypeAST *>> members)
      : name(name), StructTypeAST(members) {}
  Type *type() {
    std::vector<std::pair<std::string, Type *>> types;
    for (auto &m : members)
      types.push_back(std::make_pair(m.first, m.second->type()));
    return new NamedStructType(name, types);
  }
};

class TupleTypeAST : public TypeAST {
public:
  std::vector<TypeAST *> types;
  TupleTypeAST(std::vector<TypeAST *> types) : types(types) {}
  Type *type() {
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
  bool match(Type *type, uint *g) {
    if (TupleType *t = dynamic_cast<TupleType *>(type)) {
      if (types.size() != t->types.size())
        return false;
      for (size_t i = 0; i < types.size(); i++)
        if (!types[i]->match(t->types[i], g))
          return false;
      return true;
    }
    return false;
  }
  bool is_generic() {
    for (auto t : types)
      if (t->is_generic())
        return true;
    return false;
  }
};

class NamedTypeAST : public TypeAST {
public:
  std::string name;
  NamedTypeAST(std::string name) : name(name) {}
  Type *type() {
    if (!curr_named_types.count(name))
      error("undefined type: " + name);
    return curr_named_types[name];
  }
  bool eq(TypeAST *other) {
    NamedTypeAST *n = dynamic_cast<NamedTypeAST *>(other);
    return n && n->name == name;
  }
  bool match(Type *type, uint *g) { return this->type()->eq(type); }
  bool is_generic() { return false; }
};

class GenericAccessAST : public TypeAST {
public:
  std::string name;
  std::vector<TypeAST *> params;
  GenericAccessAST(std::string name, std::vector<TypeAST *> params)
      : name(name), params(params) {}
  Type *type() {
    if (!curr_named_generics.count(name))
      error("undefined generic type: " + name);
    std::vector<Type *> args(params.size());
    for (size_t i = 0; i < params.size(); i++)
      args[i] = params[i]->type();
    return curr_named_generics[name]->generate(args);
  }
  bool eq(TypeAST *other) {
    NamedTypeAST *n = dynamic_cast<NamedTypeAST *>(other);
    return n && n->name == name;
  }
  bool match(Type *type, uint *g) { return this->type()->eq(type); }
  bool is_generic() { return false; }
  std::string stringify() {
    std::stringstream ss;
    ss << name << "<";
    for (size_t i = 0; i < params.size(); i++) {
      if (i != 0)
        ss << ", ";
      ss << params[i]->stringify();
    }
    ss << ">";
    return ss.str();
  }
};

class TypeofAST : public TypeAST {
public:
  ExprAST *expr;
  TypeofAST(ExprAST *expr) : expr(expr) {}
  Type *type() { return expr->get_type(); }
  bool eq(TypeAST *other) { return type()->eq(other->type()); }
  bool match(Type *type, uint *g) { return this->type()->eq(type); }
  bool is_generic() { return false; }
};

class UnionTypeAST : public TypeAST {
  TypeAST *picked;

public:
  std::vector<TypeAST *> choices;
  UnionTypeAST(std::vector<TypeAST *> choices) : choices(choices) {}
  // depends on match being called before type
  Type *type() { return picked->type(); }
  bool eq(TypeAST *other) {
    UnionTypeAST *g = dynamic_cast<UnionTypeAST *>(other);
    if (!g || choices.size() != g->choices.size())
      return false;
    for (size_t i = 0; i < choices.size(); i++)
      if (!choices[i]->eq(g->choices[i]))
        return false;
    return true;
  }
  bool match(Type *type, uint *generic_count) {
    if (generic_count == nullptr) {
      for (auto &c : choices)
        if (c->match(type)) {
          picked = c;
          return true;
        }
    } else {
      for (auto &c : choices) {
        uint g_copy = *generic_count + 1;
        if (c->match(type, &g_copy)) {
          picked = c;
          *generic_count = g_copy;
          return true;
        }
      }
    }
    return false;
  }
  bool is_generic() { return true; }
  std::string stringify() {
    std::stringstream res;
    res << "(";
    for (size_t i = 0; i < choices.size(); i++) {
      if (i != 0)
        res << " | ";
      res << choices[i]->stringify();
    }
    res << ")";
    return res.str();
  }
};

class GenericTypeAST : public TypeAST {
public:
  std::string name;
  GenericTypeAST(std::string name) : name(name) {}
  Type *type() { return curr_named_types[name]; }
  bool eq(TypeAST *other) {
    GenericTypeAST *g = dynamic_cast<GenericTypeAST *>(other);
    return g && name == g->name;
  }
  bool match(Type *type, uint *generic_count) {
    curr_named_types[name] = type;
    if (generic_count)
      (*generic_count) += 1;
    return true;
  }
  bool is_generic() { return true; }
  std::string stringify() { return "generic " + name; }
};