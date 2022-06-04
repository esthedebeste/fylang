#pragma once
#include "../types.cpp"
#include "../utils.cpp"
#include "../values.cpp"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Type *get_type() = 0;
  virtual Value *gen_value() = 0;
  virtual bool is_constant() { return false; }
};

struct Scope {
  std::unordered_map<std::string, Value *> named_variables;
  Scope *parent_scope = nullptr;
  Scope(Scope *parent_scope) : parent_scope(parent_scope) {}
  Value *get_named_variable(std::string name) {
    if (named_variables.count(name))
      return named_variables[name];
    if (parent_scope)
      return parent_scope->get_named_variable(name);
    return nullptr;
  }
  void declare_variable(std::string name, Type *type) {
    named_variables[name] = new ConstValue(type, nullptr);
  }
  void set_variable(std::string name, Value *value) {
    named_variables[name] = value;
  }
};
Scope *global_scope = new Scope(nullptr);
Scope *curr_scope = global_scope;
Scope *push_scope() { return curr_scope = new Scope(curr_scope); }
Scope *pop_scope();

#include "functions.cpp"
#include "types.cpp"

Scope *pop_scope() {
  for (auto &[name, value] : curr_scope->named_variables) {
    Type *type = value->get_type();
    FunctionAST *destructor = type->get_destructor();
    if (!destructor)
      continue;
    LLVMValueRef llvm_val = value->gen_val();
    if (!llvm_val) // type phase
      continue;
    ConstValue val = ConstValue(type, llvm_val);
    destructor->gen_call({&val});
  }
  return curr_scope = curr_scope->parent_scope;
}
NumType *sizeof_type;
/// SizeofExprAST - Expression class to get the byte size of a type
class SizeofExprAST : public ExprAST {
public:
  TypeAST *type;
  SizeofExprAST(TypeAST *type) : type(type) {
    if (!sizeof_type)
      sizeof_type = new NumType(false); // uint_ptrsize
  }
  Type *get_type() { return sizeof_type; }
  Value *gen_value() {
    return new ConstValue(sizeof_type, LLVMSizeOf(type->llvm_type()));
  }
  bool is_constant() { return true; }
};
inline Value *build_malloc(Type *type) {
  if (!curr_named_functions.count("malloc"))
    error("malloc not defined before using 'new', maybe add 'include "
          "\"c/stdlib\"'?");
  return new NamedValue(curr_named_functions["malloc"]
                            ->gen_call({new SizeofExprAST(type_ast(type))})
                            ->cast_to(type->ptr()),
                        "malloc_" + type->stringify());
}

std::unordered_map<char, NumType> num_type_chars = {
    {'b', NumType(8, false, false)}, {'d', NumType(64, true, true)},
    {'f', NumType(32, true, true)},  {'i', NumType(32, false, true)},
    {'l', NumType(64, false, true)}, {'u', NumType(32, false, false)},
};
NumType num_char_to_type(char type_char, bool has_dot) {
  if (num_type_chars.count(type_char) == 0)
    error((std::string) "Invalid number type id '" + type_char + "'/"
          << (int)type_char);
  auto &num_type = num_type_chars[type_char];
  if (has_dot && !num_type.is_floating)
    error((std::string) "'" + type_char + "' (" + num_type.stringify() +
          ") type can't have decimals");
  return num_type;
}
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  union {
    long double floating;
    unsigned long long integer;
  } value;

public:
  NumType type;
  NumberExprAST(std::string val, char type_char, bool has_dot,
                unsigned int base)
      : type(num_char_to_type(type_char, has_dot)) {
    if (type.is_floating)
      if (base != 10)
        error("floating-point numbers with a base that isn't decimal aren't "
              "supported.");
      else
        value.floating = std::stold(val);
    else
      value.integer = std::stoull(val, nullptr, base);
  }
  NumberExprAST(unsigned long long val, char type_char)
      : type(num_char_to_type(type_char, false)) {
    value.integer = val;
  }
  NumberExprAST(unsigned long long val, NumType type) : type(type) {
    value.integer = val;
  }
  Type *get_type() { return &type; }
  Value *gen_value() {
    if (type.is_floating)
      return new ConstValue(&type,
                            LLVMConstReal(type.llvm_type(), value.floating));
    else
      return new IntValue(type, value.integer);
  }
  bool is_constant() { return true; }
};
Type *bool_type;
/// BoolExprAST - Expression class for boolean literals (true or false).
class BoolExprAST : public ExprAST {
  bool value;

public:
  BoolExprAST(bool value) : value(value) {
    if (!bool_type)
      bool_type = new NumType(1, false, false);
  }
  Type *get_type() { return bool_type; }
  Value *gen_value() {
    return new ConstValue(bool_type,
                          value ? LLVMConstAllOnes(bool_type->llvm_type())
                                : LLVMConstNull(bool_type->llvm_type()));
  }
  bool is_constant() { return true; }
};

class CastExprAST : public ExprAST {

public:
  ExprAST *value;
  TypeAST *to;
  CastExprAST(ExprAST *value, TypeAST *to) : value(value), to(to) {}
  Type *get_type() { return to->type(); }
  Value *gen_value() { return value->gen_value()->cast_to(to->type()); }
  bool is_constant() { return value->is_constant(); }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {

public:
  std::string name;
  VariableExprAST(std::string name) : name(name) {}
  Type *get_type() {
    if (auto var = curr_scope->get_named_variable(name))
      return var->get_type();
    else if (curr_named_functions.count(name))
      return curr_named_functions[name]->get_type();
    else
      error("Variable '" + name + "' doesn't exist.");
  }
  Value *gen_value() {
    if (auto var = curr_scope->get_named_variable(name))
      return var;
    else if (curr_named_functions.count(name))
      return curr_named_functions[name]->gen_ptr();
    else
      error("Variable '" + name + "' doesn't exist.");
  }
};

std::vector<std::pair<LLVMValueRef, ExprAST *>> inits;
void add_store_before_main(LLVMValueRef ptr, ExprAST *val) {
  inits.push_back({ptr, val});
}
extern std::unordered_set<LLVMValueRef> removed_globals; // defined in UCR
void add_stores_before_main(LLVMValueRef main_func) {
  if (inits.size() == 0)
    return; // nothing to do
  LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(main_func);
  LLVMBasicBlockRef store_block =
      LLVMAppendBasicBlock(main_func, "global_vars");
  LLVMMoveBasicBlockBefore(store_block, entry);
  LLVMPositionBuilderAtEnd(curr_builder, store_block);
  bool has_non_constant_init = false;
  for (auto &[ptr, expr] : inits)
    // UCR can remove globals, so we need to check if the global still exists
    if (removed_globals.count(ptr) == 0) {
      LLVMValueRef val = expr->gen_value()->gen_val();
      if (LLVMIsConstant(val))
        LLVMSetInitializer(ptr, val);
      else {
        LLVMBuildStore(curr_builder, val, ptr);
        has_non_constant_init = true;
      }
    }
  LLVMBuildBr(curr_builder, entry);
  if (!has_non_constant_init)
    LLVMDeleteBasicBlock(store_block);
}

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST {

public:
  std::string id;
  bool untyped;
  TypeAST *type;
  ExprAST *value;
  bool constant;
  LetExprAST(std::string id, TypeAST *type, ExprAST *value, bool constant)
      : id(id), type(type), value(value), constant(constant),
        untyped(type == nullptr) {}
  Type *get_type() {
    Type *type;
    if (untyped) {
      if (value)
        type = value->get_type();
      else
        error("Untyped valueless variable " + id);
    } else
      type = this->type->type();
    curr_scope->declare_variable(id, type);
    return type;
  }
  LLVMValueRef gen_toplevel() {
    Type *type = get_type();
    LLVMValueRef ptr =
        LLVMAddGlobal(curr_module, type->llvm_type(), id.c_str());
    Value *var_value = new BasicLoadValue(type, ptr);
    LLVMSetInitializer(ptr, LLVMConstNull(type->llvm_type()));
    if (value) {
      if (value->is_constant()) {
        LLVMValueRef val = value->gen_value()->cast_to(type)->gen_val();
        LLVMSetInitializer(ptr, val);
        if (constant)
          var_value = new ConstValueWithPtr(type, ptr, val);
      } else
        add_store_before_main(ptr, new CastExprAST(value, type_ast(type)));
    }
    curr_scope->set_variable(id, var_value);
    if (constant)
      LLVMSetGlobalConstant(ptr, true);
    return ptr;
  }
  Value *gen_value() {
    Type *type = get_type();
    if (constant) {
      if (value) {
        NamedValue *val = new NamedValue(value->gen_value()->cast_to(type), id);
        curr_scope->set_variable(id, val);
        return val;
      } else
        error("Constant variables need an initialization value");
    }
    LLVMValueRef ptr =
        LLVMBuildAlloca(curr_builder, type->llvm_type(), id.c_str());
    LLVMSetValueName2(ptr, id.c_str(), id.size());
    if (value) {
      LLVMValueRef llvm_val = value->gen_value()->cast_to(type)->gen_val();
      LLVMBuildStore(curr_builder, llvm_val, ptr);
    }
    BasicLoadValue *val = new BasicLoadValue(type, ptr);
    curr_scope->set_variable(id, val);
    return val;
  }
  LLVMValueRef gen_declare() {
    Type *type = get_type();
    LLVMValueRef global =
        LLVMAddGlobal(curr_module, type->llvm_type(), id.c_str());
    curr_scope->set_variable(id, new BasicLoadValue(type, global));
    return global;
  }
};

/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public NumberExprAST {
public:
  CharExprAST(char data) : NumberExprAST(data, NumType(8, false, false)) {}
};

/// StringExprAST - Expression class for multiple chars ("hello")
template <typename CharT> class StringExprAST : public ExprAST {
  inline static NumType char_type{NumType(sizeof(CharT) * 8, false, false)};
  ArrayType t_type;
  std::basic_string<CharT> str;

public:
  StringExprAST(std::basic_string<CharT> str)
      : str(str), t_type(&char_type, str.size()) {}
  Type *get_type() { return &t_type; }
  Value *gen_value() {
    LLVMValueRef *vals = new LLVMValueRef[str.size()];
    for (size_t i = 0; i < str.size(); i++)
      vals[i] = LLVMConstInt(char_type.llvm_type(), str[i], false);
    auto array = LLVMConstArray(char_type.llvm_type(), vals, str.size());
    return new ConstValue(&t_type, array);
  }
  bool is_constant() { return true; }
};

template <typename CharT> class CStringExprAST : public ExprAST {
  inline static NumType char_type{NumType(sizeof(CharT) * 8, false, false)};
  ArrayType t_type;
  PointerType p_type;
  std::basic_string<CharT> str;

public:
  CStringExprAST(std::basic_string<CharT> str)
      : str(str), t_type(&char_type, str.size() + 1), p_type(&this->t_type) {}
  Type *get_type() { return &p_type; }
  Value *gen_value() {
    auto len = str.size();
    LLVMValueRef *vals = new LLVMValueRef[len + 1];
    for (size_t i = 0; i < len; i++)
      vals[i] = LLVMConstInt(char_type.llvm_type(), str[i], false);
    vals[len] = LLVMConstNull(char_type.llvm_type());
    auto array = LLVMConstArray(char_type.llvm_type(), vals, len + 1);
    LLVMValueRef ptr = LLVMAddGlobal(curr_module, t_type.llvm_type(), ".c_str");
    LLVMSetInitializer(ptr, array);
    LLVMSetGlobalConstant(ptr, true);
    LLVMValueRef cast =
        LLVMBuildBitCast(curr_builder, ptr, p_type.llvm_type(), UN);
    return new ConstValue(&p_type, cast);
  }
  bool is_constant() { return true; }
};
Type *get_binop_type(int op, Type *lhs_t, Type *rhs_t);
Value *gen_binop(int op, LLVMValueRef lhs, LLVMValueRef rhs, Type *lhs_t,
                 Type *rhs_t);
LLVMValueRef gen_num_num_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               NumType *lhs_nt, NumType *rhs_nt) {
  if (lhs_nt->neq(rhs_nt))
    R = gen_num_cast(R, rhs_nt, lhs_nt);
  bool floating = lhs_nt->is_floating && rhs_nt->is_floating;
  if (floating)
    switch (op) {
    case '+':
      return LLVMBuildFAdd(curr_builder, L, R, UN);
    case '-':
      return LLVMBuildFSub(curr_builder, L, R, UN);
    case '*':
      return LLVMBuildFMul(curr_builder, L, R, UN);
    case '/':
      return LLVMBuildFDiv(curr_builder, L, R, UN);
    case '%':
      return LLVMBuildFRem(curr_builder, L, R, UN);
    case T_LAND:
    case '&':
      return LLVMBuildAnd(curr_builder, L, R, UN);
    case T_LOR:
    case '|':
      return LLVMBuildOr(curr_builder, L, R, UN);
    case '<':
      return LLVMBuildFCmp(curr_builder, LLVMRealULT, L, R, UN);
    case '>':
      return LLVMBuildFCmp(curr_builder, LLVMRealUGT, L, R, UN);
    case T_LEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealULE, L, R, UN);
    case T_GEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUGE, L, R, UN);
    case T_EQEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUEQ, L, R, UN);
    case T_NEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUNE, L, R, UN);
    default:
      error("invalid float_float binary operator '" + token_to_str(op) + "'");
    }
  else if (!lhs_nt->is_floating && !rhs_nt->is_floating) {
    bool is_signed = lhs_nt->is_signed && rhs_nt->is_signed;
    switch (op) {
    case '+':
      return LLVMBuildAdd(curr_builder, L, R, UN);
    case '-':
      return LLVMBuildSub(curr_builder, L, R, UN);
    case '*':
      return LLVMBuildMul(curr_builder, L, R, UN);
    case '/':
      return is_signed ? LLVMBuildSDiv(curr_builder, L, R, UN)
                       : LLVMBuildUDiv(curr_builder, L, R, UN);
    case '%':
      return is_signed ? LLVMBuildSRem(curr_builder, L, R, UN)
                       : LLVMBuildURem(curr_builder, L, R, UN);
    case T_LAND:
    case '&':
      return LLVMBuildAnd(curr_builder, L, R, UN);
    case T_LOR:
    case '|':
      return LLVMBuildOr(curr_builder, L, R, UN);
    case T_LSHIFT:
      return LLVMBuildShl(curr_builder, L, R, UN);
    case T_RSHIFT:
      return LLVMBuildAShr(curr_builder, L, R, UN);
    case '<':
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSLT : LLVMIntULT, L,
                           R, UN);
    case '>':
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSGT : LLVMIntUGT, L,
                           R, UN);
    case T_LEQ:
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSLE : LLVMIntULE, L,
                           R, UN);
    case T_GEQ:
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSGE : LLVMIntUGE, L,
                           R, UN);
    case T_EQEQ:
      return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntEQ, L, R, UN);
    case T_NEQ:
      return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntNE, L, R, UN);
    default:
      error("invalid int_int binary operator '" + token_to_str(op) + "'");
    }
  } else
    error("invalid float_int binary operator '" + token_to_str(op) + "'");
}
LLVMValueRef gen_ptr_num_binop(int op, LLVMValueRef ptr, LLVMValueRef num,
                               PointerType *ptr_t, NumType *num_t) {
  switch (op) {
  case '-':
    // num = 0-num
    num = LLVMBuildNeg(curr_builder, num, UN);
    /* falls through */
  case '+':
    return LLVMBuildGEP2(curr_builder, ptr_t->points_to->llvm_type(), ptr, &num,
                         1, "ptraddtmp");
  default: {
    auto num2 = LLVMBuildPtrToInt(curr_builder, ptr, num_t->llvm_type(), UN);
    return gen_num_num_binop(op, num2, num, num_t, num_t);
  }
  }
}
LLVMValueRef gen_ptr_ptr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               PointerType *lhs_ptr, PointerType *rhs_pt) {
  switch (op) {
  case T_EQEQ:
    return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntEQ, L, R, UN);
  case T_NEQ:
    return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntNE, L, R, UN);
  default: {
    NumType uint_ptrsize(false);
    return gen_num_num_binop(op, L, R, &uint_ptrsize, &uint_ptrsize);
  }
  }
}

LLVMValueRef gen_arr_arr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               ArrayType *lhs_at, ArrayType *rhs_at) {
  if (lhs_at->neq(rhs_at))
    error("Array-array comparison with different types: " +
          lhs_at->stringify() + " doesn't match " + rhs_at->stringify());
  auto arr_size = lhs_at->count;
  if (arr_size == 0)
    error("Array-array comparison with empty arrays");
  auto arr_type = lhs_at->llvm_type();
  auto res_type = get_binop_type(op, lhs_at->elem, rhs_at->elem);
  LLVMValueRef res;
  for (size_t i = 0; i < arr_size; i++) {
    auto lhs_elem = LLVMBuildExtractValue(curr_builder, L, i, UN);
    auto rhs_elem = LLVMBuildExtractValue(curr_builder, R, i, UN);
    auto elem_res =
        gen_binop(op, lhs_elem, rhs_elem, lhs_at->elem, rhs_at->elem);
    auto elem_val = elem_res->gen_val();
    if (op == T_EQEQ) {
      // a[0] == b[0] && a[1] == b[1] && ...
      if (!res)
        res = elem_val;
      else
        res = LLVMBuildAnd(curr_builder, res, elem_val, UN);
    } else {
      // ( a[0] + b[0], a[1] + b[1], ... )
      if (!res)
        res = LLVMGetUndef(arr_type);
      res = LLVMBuildInsertValue(curr_builder, res, elem_val, i, UN);
    }
  }
  return res;
}

Type *get_binop_type(int op, Type *lhs_t, Type *rhs_t) {
  TypeType lhs_tt = lhs_t->type_type();
  TypeType rhs_tt = rhs_t->type_type();

  if (binop_precedence[op] == comparison_prec)
    return BoolExprAST(true).get_type();
  else if (lhs_tt == Number && rhs_tt == Number)
    return lhs_t; // int + int returns int
  else if (lhs_tt == Pointer && rhs_tt == Number)
    // ptr + int returns offsetted ptr
    return /* ptr */ lhs_t;
  else if (lhs_tt == Number && rhs_tt == Pointer)
    // int + ptr returns offsetted ptr
    return /* ptr */ rhs_t;
  else if (lhs_tt == Array && rhs_tt == Array)
    return lhs_t; // array + array returns array
  error("Unknown op " + token_to_str(op) + " for types " + lhs_t->stringify() +
        " and " + rhs_t->stringify());
}

Value *gen_binop(int op, LLVMValueRef L, LLVMValueRef R, Type *lhs_t,
                 Type *rhs_t) {
  Type *type = get_binop_type(op, lhs_t, rhs_t);
  auto lhs_nt = dynamic_cast<NumType *>(lhs_t);
  auto rhs_nt = dynamic_cast<NumType *>(rhs_t);
  auto lhs_pt = dynamic_cast<PointerType *>(lhs_t);
  auto rhs_pt = dynamic_cast<PointerType *>(rhs_t);
  auto lhs_at = dynamic_cast<ArrayType *>(lhs_t);
  auto rhs_at = dynamic_cast<ArrayType *>(rhs_t);
  if (lhs_nt && rhs_nt)
    return new ConstValue(type, gen_num_num_binop(op, L, R, lhs_nt, rhs_nt));
  else if (lhs_nt && rhs_pt)
    return new ConstValue(type, gen_ptr_num_binop(op, R, L, rhs_pt, lhs_nt));
  else if (lhs_pt && rhs_nt)
    return new ConstValue(type, gen_ptr_num_binop(op, L, R, lhs_pt, rhs_nt));
  else if (lhs_pt && rhs_pt)
    return new ConstValue(type, gen_ptr_ptr_binop(op, L, R, lhs_pt, rhs_pt));
  else if (lhs_at && rhs_at)
    return new ConstValue(type, gen_arr_arr_binop(op, L, R, lhs_at, rhs_at));
  error("Unknown op " + token_to_str(op) + " for types " + lhs_t->stringify() +
        " and " + rhs_t->stringify());
}

class AssignExprAST : public ExprAST {
  ExprAST *LHS, *RHS;

public:
  AssignExprAST(ExprAST *LHS, ExprAST *RHS) : LHS(LHS), RHS(RHS) {}
  Type *get_type() { return LHS->get_type(); }
  Value *gen_value() {
    CastValue *val = RHS->gen_value()->cast_to(LHS->get_type());
    LLVMBuildStore(curr_builder, val->gen_val(), LHS->gen_value()->gen_ptr());
    return val;
  }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST *LHS, *RHS;

public:
  BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS)
      : op(op), LHS(LHS), RHS(RHS) {}

  Type *get_type() {
    return get_binop_type(op, LHS->get_type(), RHS->get_type());
  }

  Value *gen_value() {
    return gen_binop(op, LHS->gen_value()->gen_val(),
                     RHS->gen_value()->gen_val(), LHS->get_type(),
                     RHS->get_type());
  }
  // LLVM can constantify binary expressions if both sides are also constant.
  bool is_constant() { return LHS->is_constant() && RHS->is_constant(); }
};
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  int op;
  ExprAST *operand;

public:
  UnaryExprAST(int op, ExprAST *operand) : op(op), operand(operand) {}
  Type *get_type() {
    if (op == '*')
      if (PointerType *opt = dynamic_cast<PointerType *>(operand->get_type()))
        return opt->get_points_to();
      else
        error("* can't be used on a non-pointer type");
    else if (op == '&')
      return operand->get_type()->ptr();
    else
      return operand->get_type();
  }
  Value *gen_value() {
    Type *type = get_type();
    Value *val = operand->gen_value();
    switch (op) {
    case '!':
      // shortcut for == 0
      if (NumType *num_type = dynamic_cast<NumType *>(val->get_type()))
        return new ConstValue(
            new NumType(1, false, false),
            gen_num_num_binop(T_EQEQ, val->gen_val(),
                              LLVMConstNull(val->get_type()->llvm_type()),
                              num_type, num_type));
      else
        error("'!' unary op can only be used on numbers");
    case '-':
      // shortcut for 0 - num
      if (NumType *num_type = dynamic_cast<NumType *>(val->get_type()))
        return new ConstValue(
            num_type,
            gen_num_num_binop('-', LLVMConstNull(val->get_type()->llvm_type()),
                              val->gen_val(), num_type, num_type));
      else
        error("'-' unary op can only be used on numbers");
    case '*':
      return new BasicLoadValue(type, val->gen_val());
    case '&':
      return new ConstValue(type, val->gen_ptr());
    case T_RETURN:
      add_return(val->gen_val());
      LLVMPositionBuilderAtEnd(
          curr_builder,
          LLVMAppendBasicBlock(
              LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder)), UN));
      return val;
    default:
      error("invalid prefix unary operator '" + token_to_str(op) + "'");
    }
  }
};

/// ValueCallExprAST - For calling a value (often a function pointer)
class ValueCallExprAST : public ExprAST {
  ExprAST *called;
  std::vector<ExprAST *> args;
  bool is_ptr;
  FunctionType *get_func_type() {
    FunctionType *func_t = dynamic_cast<FunctionType *>(called->get_type());
    if (!func_t) {
      if (PointerType *ptr = dynamic_cast<PointerType *>(called->get_type()))
        func_t = dynamic_cast<FunctionType *>(ptr->get_points_to());
      else
        error("Function doesn't exist or is not a function");
    }
    return func_t;
  }

public:
  ValueCallExprAST(ExprAST *called, std::vector<ExprAST *> args)
      : called(called), args(args) {}

  Type *get_type() {
    FunctionType *func_t = get_func_type();
    if (func_t->flags.is_vararg ? args.size() < func_t->arguments.size()
                                : args.size() != func_t->arguments.size())
      error("Incorrect # arguments passed. (Expected " +
            std::to_string(func_t->arguments.size()) + ", got " +
            std::to_string(args.size()) + ")");

    return func_t->return_type;
  }
  Value *gen_value() {
    FunctionType *func_t = get_func_type();
    Value *func_v = called->gen_value();
    if (!func_v)
      error("Unknown function referenced");
    LLVMValueRef func = func_v->gen_val();
    LLVMValueRef *arg_vs = new LLVMValueRef[args.size()];
    for (size_t i = 0; i < args.size(); i++) {
      if (i < func_t->arguments.size())
        arg_vs[i] =
            args[i]->gen_value()->cast_to(func_t->arguments[i])->gen_val();
      else
        arg_vs[i] = args[i]->gen_value()->gen_val();
    }
    auto call = LLVMBuildCall2(curr_builder, func_t->llvm_type(), func, arg_vs,
                               args.size(), UN);
    LLVMSetInstructionCallConv(call, func_t->flags.call_conv);
    return new ConstValue(func_t->return_type, call);
  }
};

/// ASMExprAST - Inline assembly
class ASMExprAST : public ExprAST {
  TypeAST *type_ast;
  std::string asm_str;
  bool has_output;
  std::string out_reg;
  std::vector<std::pair<std::string, ExprAST *>> args;

public:
  ASMExprAST(TypeAST *type_ast, std::string asm_str, std::string out_reg,
             std::vector<std::pair<std::string, ExprAST *>> args)
      : type_ast(type_ast), asm_str(asm_str), has_output(true),
        out_reg(out_reg), args(args) {}
  ASMExprAST(std::string asm_str,
             std::vector<std::pair<std::string, ExprAST *>> args)
      : asm_str(asm_str), has_output(false), args(args) {}

  Type *get_type() { return has_output ? type_ast->type() : new NullType(); }
  Value *gen_value() {
    LLVMValueRef *arg_vs = new LLVMValueRef[args.size()];
    LLVMTypeRef *arg_ts = new LLVMTypeRef[args.size()];
    std::vector<std::string> constraints;
    if (has_output)
      constraints.push_back("={" + out_reg + "}");
    for (size_t i = 0; i < args.size(); i++) {
      arg_vs[i] = args[i].second->gen_value()->gen_val();
      arg_ts[i] = args[i].second->get_type()->llvm_type();
      constraints.push_back("{" + args[i].first + "}");
    }
    std::string constraints_str = "";
    for (size_t i = 0; i < constraints.size(); i++) {
      if (i != 0)
        constraints_str += ",";
      constraints_str += constraints[i];
    }
    Type *type = has_output ? type_ast->type() : new NullType();
    LLVMTypeRef functy =
        LLVMFunctionType(has_output ? type->llvm_type() : LLVMVoidType(),
                         arg_ts, args.size(), false);
    LLVMValueRef inline_asm =
        LLVMGetInlineAsm(functy, asm_str.data(), asm_str.size(),
                         constraints_str.data(), constraints_str.size(),
                         /* has side effects */ !has_output, false,
                         LLVMInlineAsmDialectATT, false);
    LLVMValueRef call = LLVMBuildCall2(curr_builder, functy, inline_asm, arg_vs,
                                       args.size(), has_output ? UN : "");
    return new ConstValue(
        type, has_output ? call : LLVMConstNull(NullType().llvm_type()));
  }
};

class NameCallExprAST : public ExprAST {
public:
  std::string name;
  std::vector<ExprAST *> args;
  NameCallExprAST(std::string name, std::vector<ExprAST *> args)
      : name(name), args(args) {}
  Type *get_type() {
    if (curr_named_functions.count(name))
      return curr_named_functions[name]->get_type(args)->return_type;
    else
      return ValueCallExprAST(new VariableExprAST(name), args).get_type();
  }
  Value *gen_value() {
    if (curr_named_functions.count(name))
      return curr_named_functions[name]->gen_call(args);
    else
      return ValueCallExprAST(new VariableExprAST(name), args).gen_value();
  }
};

/// IndexExprAST - Expression class for accessing indexes (a[0]).
class IndexExprAST : public ExprAST {
  ExprAST *value;
  ExprAST *index;

public:
  IndexExprAST(ExprAST *value, ExprAST *index) : value(value), index(index) {}
  Type *get_type() {
    Type *base_type = value->get_type();
    if (PointerType *p_type = dynamic_cast<PointerType *>(base_type))
      return p_type->get_points_to();
    else if (ArrayType *arr_type = dynamic_cast<ArrayType *>(base_type))
      return arr_type->get_elem_type();
    else
      error("Invalid index, type not arrayish.\n"
            "Expected: array | pointer \nGot: " +
            base_type->stringify());
  }

  Value *gen_value() {
    Type *type = get_type();
    Value *val = value->gen_value();
    LLVMValueRef index_v = index->gen_value()->gen_val();
    Type *base_type = value->get_type();
    if (PointerType *p_type = dynamic_cast<PointerType *>(base_type)) {
      return new BasicLoadValue(
          type,
          LLVMBuildGEP2(curr_builder, p_type->get_points_to()->llvm_type(),
                        val->gen_val(), &index_v, 1, UN));
    } else if (ArrayType *arr_type = dynamic_cast<ArrayType *>(base_type)) {
      if (val->has_ptr()) {
        LLVMValueRef index[2] = {LLVMConstNull(NumType(false).llvm_type()),
                                 index_v};
        return new BasicLoadValue(
            type, LLVMBuildGEP2(curr_builder, arr_type->llvm_type(),
                                val->gen_ptr(), index, 2, UN));
      } else if (LLVMIsAConstantInt(index_v)) {
        return new ConstValue(
            type, LLVMBuildExtractValue(curr_builder, val->gen_val(),
                                        LLVMConstIntGetZExtValue(index_v), UN));
      } else
        error("Can't index an array that doesn't have a pointer");
    }
    error("Invalid index, type not arrayish.\n"
          "Expected: array | pointer \nGot: " +
          base_type->stringify());
  }
};

/// NumAccessExprAST - Expression class for accessing indexes on Tuples (a.0).
class NumAccessExprAST : public ExprAST {
  bool is_ptr;
  TupleType *source_type;

public:
  unsigned int index;
  ExprAST *source;
  NumAccessExprAST(unsigned int index, ExprAST *source)
      : index(index), source(source) {}

  Type *get_type() {
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    }
    source_type = dynamic_cast<TupleType *>(st);
    return source_type->get_elem_type(index);
  }

  Value *gen_value() {
    Type *type = get_type();
    Value *src = source->gen_value();
    // If src is a struct-pointer (*String) then access on the value, if src
    // is a struct-value (String) then access on the pointer to where it's
    // stored.
    LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
    return new BasicLoadValue(
        type, LLVMBuildStructGEP2(curr_builder, source_type->llvm_type(),
                                  struct_ptr, index, UN));
  }
};

/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  bool is_ptr;
  StructType *source_type;
  unsigned int index;

public:
  std::string key;
  ExprAST *source;
  PropAccessExprAST(std::string key, ExprAST *source)
      : key(key), source(source) {}

  Type *get_type() {
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    } else
      is_ptr = false;
    StructType *struct_t = dynamic_cast<StructType *>(st);
    if (!struct_t)
      error("Invalid property access for key '" + key + "', " +
            st->stringify() + " is not a struct.");
    index = struct_t->get_index(key);
    Type *type = struct_t->get_elem_type(index);
    source_type = struct_t;
    return type;
  }

  Value *gen_value() {
    Type *type = get_type();
    Value *src = source->gen_value();
    if (!is_ptr && !src->has_ptr())
      return new ConstValue(
          type, LLVMBuildExtractValue(curr_builder, src->gen_val(), index, UN));
    // If src is a struct-pointer (*String) then access on the value, if src
    // is a struct-value (String) then access on the pointer to where it's
    // stored.
    LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
    return new BasicLoadValue(
        type, LLVMBuildStructGEP2(curr_builder, source_type->llvm_type(),
                                  struct_ptr, index, UN));
  }
};

struct ExtAndPtr {
  MethodAST *extension;
  bool is_ptr;
};
/// MethodCallExprAST - Expression class for calling methods (a.len()).
class MethodCallExprAST : public ExprAST {
  ExtAndPtr get_extension() {
    auto extension = get_method(source->get_type(), name);
    if (!extension)
      return {get_method(source->get_type()->ptr(), name), true};
    else
      return {extension, false};
    return {nullptr, false};
  }

public:
  std::string name;
  ExprAST *source;
  std::vector<ExprAST *> args;
  MethodCallExprAST(std::string name, ExprAST *source,
                    std::vector<ExprAST *> args)
      : name(name), source(source), args(args) {}

  Type *get_type() {
    auto ext = get_extension();
    if (ext.extension != nullptr)
      return ext.extension
          ->get_type(args, ext.is_ptr ? new UnaryExprAST('&', source) : source)
          ->return_type;
    else
      return ValueCallExprAST(new PropAccessExprAST(name, source), args)
          .get_type();
  }
  Value *gen_value() {
    auto ext = get_extension();
    if (ext.extension != nullptr)
      return ext.extension->gen_call(
          args, ext.is_ptr ? new UnaryExprAST('&', source) : source);
    else
      return ValueCallExprAST(new PropAccessExprAST(name, source), args)
          .gen_value();
  }
};

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {

public:
  TypeAST *s_type;
  std::vector<std::pair<std::string, ExprAST *>> fields;
  bool is_new;
  NewExprAST(TypeAST *s_type,
             std::vector<std::pair<std::string, ExprAST *>> fields, bool is_new)
      : s_type(s_type), fields(fields), is_new(is_new) {}
  Type *get_type() { return is_new ? s_type->type()->ptr() : s_type->type(); }

  Value *gen_value() {
    StructType *st = dynamic_cast<StructType *>(s_type->type());
    if (!st)
      error("Cannot create instance of non-struct type " +
            s_type->type()->stringify());
    LLVMValueRef agg = LLVMConstNull(st->llvm_type());
    for (size_t i = 0; i < fields.size(); i++) {
      auto &[key, value] = fields[i];
      size_t index = key == "" ? i : st->get_index(key);
      agg = LLVMBuildInsertValue(
          curr_builder, agg,
          value->gen_value()->cast_to(st->get_elem_type(index))->gen_val(),
          index, key.c_str());
    }
    if (is_new) {
      LLVMValueRef ptr = build_malloc(st)->gen_val();
      LLVMBuildStore(curr_builder, agg, ptr);
      return new ConstValue(s_type->type()->ptr(), ptr);
    } else {
      return new ConstValue(s_type->type(), agg);
    }
  }
  bool is_constant() {
    if (is_new)
      return false;
    for (auto &[key, value] : fields)
      if (!value->is_constant())
        return false;
    return true;
  }
};

class TupleExprAST : public ExprAST {
  std::vector<ExprAST *> values;
  TupleType *t_type;

public:
  bool is_new = false;
  TupleExprAST(std::vector<ExprAST *> values) : values(values) {}

  Type *get_type() {
    std::vector<Type *> types;
    for (auto &value : values)
      types.push_back(value->get_type());
    t_type = new TupleType(types);
    if (is_new)
      return t_type->ptr();
    else
      return t_type;
  }

  Value *gen_value() {
    if (is_constant()) {
      LLVMValueRef *vals = new LLVMValueRef[values.size()];
      for (size_t i = 0; i < values.size(); i++)
        vals[i] = values[i]->gen_value()->gen_val();
      return new ConstValue(get_type(),
                            LLVMConstStruct(vals, values.size(), true));
    }
    Type *type = get_type();
    if (is_new) {
      LLVMValueRef ptr = build_malloc(t_type)->gen_val();
      for (size_t i = 0; i < values.size(); i++) {
        auto value = values[i]->gen_value()->gen_val();
        LLVMValueRef set_ptr =
            LLVMBuildStructGEP2(curr_builder, t_type->llvm_type(), ptr, i, UN);
        LLVMBuildStore(curr_builder, value, set_ptr);
      }
      return new ConstValue(t_type->ptr(), ptr);
    } else {
      LLVMValueRef agg = LLVMConstNull(t_type->llvm_type());
      for (size_t i = 0; i < values.size(); i++) {
        auto value = values[i]->gen_value()->gen_val();
        agg = LLVMBuildInsertValue(curr_builder, agg, value, i, UN);
      }
      return new ConstValue(t_type, agg);
    }
  }
  bool is_constant() {
    if (is_new)
      return false;
    for (auto &value : values)
      if (!value->is_constant())
        return false;
    return true;
  }
};

class BlockExprAST : public ExprAST {

public:
  std::vector<ExprAST *> exprs;
  BlockExprAST(std::vector<ExprAST *> exprs) : exprs(exprs) {
    if (exprs.size() == 0)
      error("block can't be empty.");
  }
  Type *get_type() {
    push_scope();
    // initialize previous exprs
    for (size_t i = 0; i < exprs.size() - 1; i++)
      exprs[i]->get_type();
    Type *type = exprs.back()->get_type();
    pop_scope();
    return type;
  }
  Value *gen_value() {
    push_scope();
    // generate code for all exprs and only return last expr
    for (size_t i = 0; i < exprs.size() - 1; i++)
      exprs[i]->gen_value();
    Value *value = exprs.back()->gen_value();
    pop_scope();
    return value;
  }
};

ConstValue *null_value(Type *type) {
  return new ConstValue(type, LLVMConstNull(type->llvm_type()));
}
/// NullExprAST - null
class NullExprAST : public ExprAST {
public:
  Type *type;
  NullExprAST(Type *type) : type(type) {}
  NullExprAST() : type(new NullType()) {}
  Type *get_type() { return type; }
  Value *gen_value() { return null_value(type); }
  bool is_constant() { return true; }
};

class TypeAssertExprAST : public ExprAST {
public:
  TypeAST *a;
  TypeAST *b;
  TypeAssertExprAST(TypeAST *a, TypeAST *b) : a(a), b(b) {}
  Type *get_type() { return new NullType(); }
  Value *gen_value() {
    if (a->eq(b))
      return null_value(get_type());
    else
      error("Type mismatch in Type assertion, " + a->stringify() +
            " != " + b->stringify());
  }
};

class TypeDumpExprAST : public ExprAST {
public:
  TypeAST *type;
  TypeDumpExprAST(TypeAST *type) : type(type) {}
  Type *get_type() { return new NullType(); }
  Value *gen_value() {
    std::cout << "[DUMP] Dumped type: " << type->stringify();
    return null_value(get_type());
  }
};

/// TypeIfExprAST - Expression class for ifs based on type.
class TypeIfExprAST : public ExprAST {
  ExprAST *pick() { return b->match(a->type()) ? then : elze; }

public:
  ExprAST *then, *elze;
  TypeAST *a, *b;
  TypeIfExprAST(TypeAST *a, TypeAST *b, ExprAST *then,
                // elze because else cant be a variable name lol
                ExprAST *elze)
      : a(a), b(b), then(then), elze(elze) {}

  Type *get_type() { return pick()->get_type(); }
  Value *gen_value() { return pick()->gen_value(); }
  bool is_constant() { return pick()->is_constant(); }
};

/// OrExprAST - Expression class for a short-circuiting or
class OrExprAST : public ExprAST {
public:
  ExprAST *left, *right;
  OrExprAST(ExprAST *left, ExprAST *right) : left(left), right(right) {}

  Type *get_type() {
    auto left_t = left->get_type();
    auto right_t = right->get_type();
    if (left_t->neq(right_t))
      error("or's left and right side don't have the same type, " +
            left_t->stringify() + " does not match " + right_t->stringify() +
            ".");
    return left_t;
  }

  Value *gen_value() {
    auto type = get_type();
    auto left_bb = LLVMGetInsertBlock(curr_builder);
    auto left = this->left->gen_value();
    // cast to bool
    auto left_bool = left->cast_to(new NumType(1, false, false))->gen_val();
    left_bb = LLVMGetInsertBlock(curr_builder);
    auto func = LLVMGetBasicBlockParent(left_bb);
    auto right_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
    auto merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    // if left is true, skip right
    LLVMBuildCondBr(curr_builder, left_bool, merge_bb, right_bb);
    LLVMPositionBuilderAtEnd(curr_builder, right_bb);
    auto right = this->right->gen_value();
    LLVMBuildBr(curr_builder, merge_bb);
    // right can change the current block, update then_bb for the PHI.
    right_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    return gen_phi(left_bb, left, right_bb, right);
  }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
  ExprAST *cond, *then, *elze;
  bool null_else;
  Type *type;
  void init() {
    Type *then_t = then->get_type();
    type = then_t;
    if (null_else)
      elze = new NullExprAST(type);
    Type *else_t = elze->get_type();
    if (then_t->neq(else_t))
      error("conditional's then and else side don't have the same type, " +
            then_t->stringify() + " does not match " + else_t->stringify() +
            ".");
  }
  IfExprAST(ExprAST *cond, ExprAST *then,
            // elze because else cant be a variable name lol
            ExprAST *elze)
      : cond(cond), then(then), elze(elze), null_else(elze == nullptr) {}

  Type *get_type() {
    init();
    return type;
  }

  Value *gen_value() {
    init();
    // cast to bool
    LLVMValueRef cond_v =
        cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
    LLVMBasicBlockRef then_bb =
        LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
    LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    // if
    LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
    // then
    LLVMPositionBuilderAtEnd(curr_builder, then_bb);
    Value *then_v = then->gen_value();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'then' can change the current block, update then_bb for the
    // PHI.
    then_bb = LLVMGetInsertBlock(curr_builder);
    // else
    LLVMAppendExistingBasicBlock(func, else_bb);
    LLVMPositionBuilderAtEnd(curr_builder, else_bb);
    Value *else_v = elze->gen_value();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'else' can change the current block, update else_bb for the
    // PHI.
    else_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    return gen_phi(then_bb, then_v, else_bb, else_v);
  }
};

/// WhileExprAST - Expression class for while loops.
class WhileExprAST : public IfExprAST {
public:
  using IfExprAST::IfExprAST; // inherit constructor from IfExprAST
  Value *gen_value() {
    init();
    // cast to bool
    LLVMValueRef cond_v =
        cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
    LLVMBasicBlockRef then_bb =
        LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
    LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    // while
    LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
    // then
    LLVMPositionBuilderAtEnd(curr_builder, then_bb);
    Value *then_v = then->gen_value();
    // cast to bool
    LLVMValueRef cond_v2 =
        cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
    LLVMBuildCondBr(curr_builder, cond_v2, then_bb, merge_bb);
    // Codegen of 'then' can change the current block, update then_bb for the
    // PHI.
    then_bb = LLVMGetInsertBlock(curr_builder);
    // else
    LLVMAppendExistingBasicBlock(func, else_bb);
    LLVMPositionBuilderAtEnd(curr_builder, else_bb);
    Value *else_v = elze->gen_value();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'else' can change the current block, update else_bb for the
    // PHI.
    else_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    return gen_phi(then_bb, then_v, else_bb, else_v);
  }
};

class ForExprAST : public IfExprAST {
  ExprAST *init;
  ExprAST *post;

public:
  ForExprAST(ExprAST *init, ExprAST *cond, ExprAST *body, ExprAST *post,
             ExprAST *elze)
      : init(init), post(post), IfExprAST(cond, body, elze) {}

  Type *get_type() {
    init->get_type();
    return IfExprAST::get_type();
  }
  Value *gen_value() {
    init->gen_value(); // let i = 0
    IfExprAST::init();
    // cast to bool
    LLVMValueRef cond_v =
        cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
    LLVMBasicBlockRef then_bb =
        LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
    LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
    // for
    LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
    // then
    LLVMPositionBuilderAtEnd(curr_builder, then_bb);
    Value *then_v = then->gen_value();
    post->gen_value(); // i = i + 1
    // cast to bool
    LLVMValueRef cond_v2 =
        cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
    LLVMBuildCondBr(curr_builder, cond_v2, then_bb, merge_bb);
    // Codegen of 'then' can change the current block, update then_bb for the
    // PHI.
    then_bb = LLVMGetInsertBlock(curr_builder);
    // else
    LLVMAppendExistingBasicBlock(func, else_bb);
    LLVMPositionBuilderAtEnd(curr_builder, else_bb);
    Value *else_v = elze->gen_value();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'else' can change the current block, update else_bb for the
    // PHI.
    else_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    return gen_phi(then_bb, then_v, else_bb, else_v);
  }
};

class TypeDefAST {
public:
  virtual ~TypeDefAST() {}
  virtual void gen_toplevel() = 0;
};

class AbsoluteTypeDefAST : public TypeDefAST {
  std::string name;
  TypeAST *type;

public:
  AbsoluteTypeDefAST(std::string name, TypeAST *type)
      : name(name), type(type) {}
  void gen_toplevel() { curr_named_types[name] = type->type(); }
};

class GenericTypeDefAST : public TypeDefAST {
public:
  std::string name;
  std::vector<std::string> params;
  TypeAST *type;
  GenericTypeDefAST(std::string name, std::vector<std::string> params,
                    TypeAST *type)
      : name(name), params(params), type(type) {}
  void gen_toplevel() { curr_named_generics[name] = new Generic(params, type); }
};

/// DeclareExprAST - Expression class for defining a declare.
class DeclareExprAST {
  LetExprAST *let = nullptr;
  FunctionAST *func = nullptr;

public:
  DeclareExprAST(LetExprAST *let) : let(let) {
    curr_scope->declare_variable(let->id, let->get_type());
  }
  DeclareExprAST(FunctionAST *func) : func(func) {
    curr_named_functions[func->name] = func;
  }
  LLVMValueRef gen_toplevel() {
    if (let)
      return let->gen_declare();
    return nullptr;
  }
};