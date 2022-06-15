#include "types.h"
#include "asts.h"

TypeAST::~TypeAST() {}
LLVMTypeRef TypeAST::llvm_type() { return type()->llvm_type(); }
TypeType TypeAST::type_type() { return type()->type_type(); }
bool TypeAST::castable_from(Type *type) {
  if (is_generic())
    return match(type);
  return match(type) || type->castable_to(this->type());
}
bool TypeAST::operator==(TypeAST *other) { return eq(other); }
bool TypeAST::neq(TypeAST *other) { return !eq(other); };
std::string TypeAST::stringify() { return type()->stringify(); }
PointerType *TypeAST::ptr() { return type()->ptr(); }

Generic::Generic(std::vector<std::string> params, TypeAST *ast)
    : params(params), ast(ast) {}
Type *Generic::generate(std::vector<Type *> args) {
  if (args.size() != params.size())
    error("wrong number of arguments to generic type");
  for (size_t i = 0; i < params.size(); i++)
    curr_scope->set_type(params[i], args[i]);
  Type *type = ast->type();
  return type;
}

static std::unordered_set<std::string> curr_generic_args;
bool Generic::match(Type *type, uint *g) {
  for (auto param : params)
    curr_generic_args.insert(param);
  auto res = ast->match(type, g);
  curr_generic_args.clear();
  return res;
}

AbsoluteTypeAST::AbsoluteTypeAST(Type *typ) : typ(typ) {}
Type *AbsoluteTypeAST::type() { return typ; }
bool AbsoluteTypeAST::eq(TypeAST *other) { return typ->eq(other->type()); }
bool AbsoluteTypeAST::match(Type *type, uint *g) { return typ->eq(type); }
bool AbsoluteTypeAST::is_generic() { return false; }

UnaryTypeAST::UnaryTypeAST(int opc, TypeAST *operand)
    : opc(opc), operand(operand) {}
Type *UnaryTypeAST::type() {
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
      error("use of 'signed' type operator with non-number on right-hand side");
  default:
    error("unimplemented unary op " + token_to_str(opc));
  }
}
bool UnaryTypeAST::eq(TypeAST *other) {
  if (UnaryTypeAST *u = dynamic_cast<UnaryTypeAST *>(other))
    return opc == u->opc && operand->eq(u->operand);
  else if (AbsoluteTypeAST *a = dynamic_cast<AbsoluteTypeAST *>(other))
    return match(a->typ, nullptr);
  return false;
}
bool UnaryTypeAST::match(Type *type, uint *g) {
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
             operand->match(new NumType(num->bits, num->is_floating, true), g);
    else
      return false;
  case T_SIGNED:
    if (NumType *num = dynamic_cast<NumType *>(type))
      return num->is_signed &&
             operand->match(new NumType(num->bits, num->is_floating, true), g);
    else
      return false;
  }
  error("unimplemented unary op " + token_to_str(opc));
}
bool UnaryTypeAST::is_generic() { return operand->is_generic(); }
std::string UnaryTypeAST::stringify() {
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

FunctionTypeAST::FunctionTypeAST(TypeAST *return_type,
                                 std::vector<TypeAST *> args, FuncFlags flags)
    : return_type(return_type), args(args), flags(flags) {}
FunctionType *FunctionTypeAST::func_type() {
  std::vector<Type *> arg_types;
  for (auto arg : args)
    arg_types.push_back(arg->type());
  return new FunctionType(return_type ? return_type->type() : nullptr,
                          arg_types, flags);
}
Type *FunctionTypeAST::type() { return func_type(); }
bool FunctionTypeAST::eq(TypeAST *other) {
  FunctionTypeAST *f = dynamic_cast<FunctionTypeAST *>(other);
  if (f == nullptr || args.size() != f->args.size() || flags.neq(f->flags))
    return false;
  for (size_t i = 0; i < args.size(); i++)
    if (!args[i]->eq(f->args[i]))
      return false;
  return return_type->eq(f->return_type);
}
bool FunctionTypeAST::match(Type *type, uint *g) {
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
bool FunctionTypeAST::is_generic() {
  for (auto arg : args)
    if (arg->is_generic())
      return true;
  return return_type == nullptr ? false : return_type->is_generic();
}

ArrayTypeAST::ArrayTypeAST(TypeAST *elem, unsigned int count)
    : elem(elem), count(count) {}
Type *ArrayTypeAST::type() { return new ArrayType(elem->type(), count); }
bool ArrayTypeAST::eq(TypeAST *other) {
  if (ArrayTypeAST *a = dynamic_cast<ArrayTypeAST *>(other))
    return elem->eq(a->elem) && count == a->count;
  return false;
}
bool ArrayTypeAST::match(Type *type, uint *g) {
  if (ArrayType *a = dynamic_cast<ArrayType *>(type))
    return this->elem->match(a->elem, g) && count == a->count;
  return false;
}
bool ArrayTypeAST::is_generic() { return elem->is_generic(); }

GenericArrayTypeAST::GenericArrayTypeAST(TypeAST *elem, std::string count_name)
    : elem(elem), count_name(count_name) {}
Type *GenericArrayTypeAST::type() {
  IntValue *count =
      dynamic_cast<IntValue *>(curr_scope->get_variable(count_name));
  if (count == nullptr)
    error("Generic array not initialized properly, " << count_name
                                                     << " is undefined.");
  return new ArrayType(elem->type(), count->val);
}
bool GenericArrayTypeAST::eq(TypeAST *other) {
  if (GenericArrayTypeAST *a = dynamic_cast<GenericArrayTypeAST *>(other))
    return elem->eq(a->elem) && count_name == a->count_name;
  return false;
}
bool GenericArrayTypeAST::match(Type *type, uint *generic_count) {
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
bool GenericArrayTypeAST::is_generic() { return true; }
std::string GenericArrayTypeAST::stringify() {
  return elem->stringify() + "[generic " + count_name + "]";
}

StructTypeAST::StructTypeAST(
    std::vector<std::pair<std::string, TypeAST *>> members)
    : members(members) {}
Type *StructTypeAST::type() {
  std::vector<std::pair<std::string, Type *>> types;
  for (auto &m : members)
    types.push_back(std::make_pair(m.first, m.second->type()));
  return new StructType(types);
}
bool StructTypeAST::eq(TypeAST *other) {
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
bool StructTypeAST::match(Type *type, uint *g) {
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
bool StructTypeAST::is_generic() {
  for (auto &m : members)
    if (m.second->is_generic())
      return true;
  return false;
}

NamedStructTypeAST::NamedStructTypeAST(
    std::string name, std::vector<std::pair<std::string, TypeAST *>> members)
    : name(name), StructTypeAST(members) {}
Type *NamedStructTypeAST::type() {
  std::vector<std::pair<std::string, Type *>> types;
  for (auto &m : members)
    types.push_back(std::make_pair(m.first, m.second->type()));
  return new NamedStructType(name, types);
}

TupleTypeAST::TupleTypeAST(std::vector<TypeAST *> types) : types(types) {}
Type *TupleTypeAST::type() {
  std::vector<Type *> fields(types.size());
  for (size_t i = 0; i < types.size(); i++)
    fields[i] = types[i]->type();
  return new TupleType(fields);
}
bool TupleTypeAST::eq(TypeAST *other) {
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
bool TupleTypeAST::match(Type *type, uint *g) {
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
bool TupleTypeAST::is_generic() {
  for (auto t : types)
    if (t->is_generic())
      return true;
  return false;
}

NamedTypeAST::NamedTypeAST(std::string name) : name(name) {}
Type *NamedTypeAST::type() {
  if (auto type = curr_scope->get_type(name))
    return type;
  else
    error("undefined type: " + name);
}
bool NamedTypeAST::eq(TypeAST *other) {
  NamedTypeAST *n = dynamic_cast<NamedTypeAST *>(other);
  return n && n->name == name;
}
bool NamedTypeAST::match(Type *type, uint *g) {
  if (curr_generic_args.contains(name)) {
    curr_scope->set_type(name, type);
    if (g)
      (*g) += 1;
    return true;
  }

  return this->type()->eq(type);
}
bool NamedTypeAST::is_generic() { return false; }

GenericAccessAST::GenericAccessAST(std::string name,
                                   std::vector<TypeAST *> params)
    : name(name), params(params) {}
Type *GenericAccessAST::type() {
  auto generic = get_generic(name);
  if (!generic)
    error("undefined generic type: " + name);
  std::vector<Type *> args(params.size());
  for (size_t i = 0; i < params.size(); i++)
    args[i] = params[i]->type();
  return generic->generate(args);
}
bool GenericAccessAST::eq(TypeAST *other) {
  NamedTypeAST *n = dynamic_cast<NamedTypeAST *>(other);
  return n && n->name == name;
}
bool GenericAccessAST::match(Type *type, uint *g) {
  if (auto generic = get_generic(name))
    return generic->match(type, g);
  else
    error("undefined generic type: " + name);
}
bool GenericAccessAST::is_generic() { return false; }
std::string GenericAccessAST::stringify() {
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

TypeofAST::TypeofAST(ExprAST *expr) : expr(expr) {}
Type *TypeofAST::type() { return expr->get_type(); }
bool TypeofAST::eq(TypeAST *other) { return type()->eq(other->type()); }
bool TypeofAST::match(Type *type, uint *g) { return this->type()->eq(type); }
bool TypeofAST::is_generic() { return false; }

UnionTypeAST::UnionTypeAST(std::vector<TypeAST *> choices) : choices(choices) {}
// depends on match being called before type
Type *UnionTypeAST::type() { return picked->type(); }
bool UnionTypeAST::eq(TypeAST *other) {
  UnionTypeAST *g = dynamic_cast<UnionTypeAST *>(other);
  if (!g || choices.size() != g->choices.size())
    return false;
  for (size_t i = 0; i < choices.size(); i++)
    if (!choices[i]->eq(g->choices[i]))
      return false;
  return true;
}
bool UnionTypeAST::match(Type *type, uint *generic_count) {
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
bool UnionTypeAST::is_generic() { return true; }
std::string UnionTypeAST::stringify() {
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

GenericTypeAST::GenericTypeAST(std::string name) : name(name) {}
Type *GenericTypeAST::type() { return curr_scope->get_type(name); }
bool GenericTypeAST::eq(TypeAST *other) {
  GenericTypeAST *g = dynamic_cast<GenericTypeAST *>(other);
  return g && name == g->name;
}
bool GenericTypeAST::match(Type *type, uint *generic_count) {
  curr_scope->set_type(name, type);
  if (generic_count)
    (*generic_count) += 1;
  return true;
}
bool GenericTypeAST::is_generic() { return true; }
std::string GenericTypeAST::stringify() { return "generic " + name; }