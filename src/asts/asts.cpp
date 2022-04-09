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
};

class ValueAndType : public ExprAST {
public:
  Value *value;
  Type *type;
  ValueAndType(Type *type) : type(type) {}
  Type *get_type() { return type; };
  Value *gen_value() { return value; };
};

static std::unordered_map<std::string, ValueAndType *> curr_named_variables;
#define type_string std::pair<Type *, std::string>
template <typename A, typename B> struct std::hash<std::pair<A, B>> {
  size_t operator()(const std::pair<A, B> type) const {
    return std::hash<A>()(type.first) ^ std::hash<B>()(type.second);
  }
};
template <> struct std::equal_to<type_string> {
  bool operator()(const type_string &p1, const type_string &p2) const {
    return p1.first->eq(p2.first) && p1.second == p2.second;
  }
};

static std::unordered_map<type_string, ValueAndType *> curr_extension_methods;
#undef type_string
static FunctionType *curr_func_type;

#include "functions.cpp"
#include "types.cpp"

NumType *num_char_to_type(char type_char, bool has_dot) {
  switch (type_char) {
  case 'd':
    return new NumType(64, true, true);
  case 'f':
    return new NumType(32, true, true);
  case 'i':
    if (has_dot)
      error("'i' (int32) type can't have a '.'");
    return new NumType(32, false, true);
  case 'u':
    if (has_dot)
      error("'u' (uint32) type can't have a '.'");
    return new NumType(32, false, false);
  case 'l':
    if (has_dot)
      error("'l' (long, int64) type can't have a '.'");
    return new NumType(64, false, true);
  case 'b':
    if (has_dot)
      error("'b' (byte, uint8) type can't have a '.'");
    return new NumType(8, false, false);
  default:
    error("Invalid number type id '" + std::string(1, type_char) + "'");
  }
}
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  std::string val;
  unsigned int base;
  NumType *type;

public:
  NumberExprAST(std::string val, char type_char, bool has_dot,
                unsigned int base)
      : val(val), base(base) {
    type = num_char_to_type(type_char, has_dot);
  }
  NumberExprAST(unsigned int val, char type_char) : base(10) {
    type = num_char_to_type(type_char, false);
    this->val = std::to_string(val);
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    LLVMValueRef num;
    if (type->is_floating)
      if (base != 10) {
        error("floating-point numbers with a base that isn't decimal aren't "
              "supported.");
      } else
        num = LLVMConstRealOfStringAndSize(type->llvm_type(), val.c_str(),
                                           val.size());
    else
      num = LLVMConstIntOfStringAndSize(type->llvm_type(), val.c_str(),
                                        val.size(), base);
    return new ConstValue(type, num);
  }
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
};

class CastExprAST : public ExprAST {

public:
  ExprAST *value;
  TypeAST *to;
  CastExprAST(ExprAST *value, TypeAST *to) : value(value), to(to) {}
  Type *get_type() { return to->type(); }
  Value *gen_value() { return value->gen_value()->cast_to(to->type()); }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  ValueAndType *vt;

public:
  std::string name;
  VariableExprAST(std::string name) : name(name) {}
  ValueAndType *get_vt() {
    vt = curr_named_variables[name];
    if (!vt)
      error("Variable '" + name + "' doesn't exist.");
    return vt;
  }
  Type *get_type() { return get_vt()->type; }
  Value *gen_value() { return get_vt()->value; }
};

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST, public TopLevelAST {
  bool defined = false;
  void init() {
    if (defined)
      return;
    if (type) {
      curr_named_variables[id] = new ValueAndType(type->type());
    } else if (value != nullptr) {
      type = type_ast(value->get_type());
      curr_named_variables[id] = new ValueAndType(type->type());
    } else
      error("Untyped valueless variable " + id);
    defined = true;
  }

public:
  std::string id;
  TypeAST *type;
  ExprAST *value;
  bool constant;
  LetExprAST(std::string id, TypeAST *type, ExprAST *value, bool constant)
      : id(id), type(type), value(value), constant(constant) {}
  Type *get_type() {
    init();
    return type->type();
  }
  LLVMValueRef gen_toplevel() {
    init();
    LLVMValueRef ptr =
        LLVMAddGlobal(curr_module, type->llvm_type(), id.c_str());
    if (value) {
      LLVMValueRef val = value->gen_value()->gen_val();
      if (LLVMIsAConstant(val))
        LLVMSetInitializer(ptr, val);
      else
        error("Global variable needs a constant value inside it");
    }
    if (constant)
      LLVMSetGlobalConstant(ptr, true);
    curr_named_variables[id]->value = new BasicLoadValue(ptr, type->type());
    return ptr;
  }
  Value *gen_value() {
    init();
    if (constant) {
      if (value)
        return curr_named_variables[id]->value =
                   new NamedValue(value->gen_value(), id);
      else
        error("Constant variables need an initialization value");
    }
    LLVMValueRef ptr =
        LLVMBuildAlloca(curr_builder, type->llvm_type(), id.c_str());
    LLVMSetValueName2(ptr, id.c_str(), id.size());
    if (value) {
      LLVMValueRef llvm_val =
          value->gen_value()->cast_to(type->type())->gen_val();
      LLVMBuildStore(curr_builder, llvm_val, ptr);
    }
    return curr_named_variables[id]->value =
               new BasicLoadValue(ptr, type->type());
  }
  LLVMValueRef gen_declare() {
    init();
    LLVMValueRef global =
        LLVMAddGlobal(curr_module, type->llvm_type(), id.c_str());
    curr_named_variables[id]->value = new BasicLoadValue(global, type->type());
    return global;
  }
};

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
};

static Type *char_type;
/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public ExprAST {
  char charr;

public:
  CharExprAST(char charr) : charr(charr) {
    if (!char_type)
      char_type = new NumType(8, false, false);
  }
  Type *get_type() { return char_type; }
  Value *gen_value() {
    return new ConstValue(char_type,
                          LLVMConstInt(char_type->llvm_type(), charr, false));
  }
};

PointerType *string_type;
/// StringExprAST - Expression class for multiple chars ("hello")
class StringExprAST : public ExprAST {
  std::string str;
  ArrayType *t_type;

public:
  StringExprAST(std::string str) : str(str) {
    if (!char_type)
      char_type = new NumType(8, false, false);
    if (!string_type)
      string_type = char_type->ptr();
    t_type = new ArrayType(char_type, str.size() + 1 /* zero-byte */);
  }
  Type *get_type() { return string_type; }
  Value *gen_value() {
    LLVMValueRef data = LLVMConstString(str.c_str(), str.size(), false);
    LLVMValueRef glob = LLVMAddGlobal(curr_module, t_type->llvm_type(), ".str");
    LLVMSetInitializer(glob, data);
    LLVMSetLinkage(glob, LLVMInternalLinkage);
    LLVMSetUnnamedAddress(glob, LLVMGlobalUnnamedAddr);
    LLVMValueRef zeros[2] = {
        LLVMConstInt(NumType(false).llvm_type(), 0, false),
        LLVMConstInt(NumType(false).llvm_type(), 0, false)};
    // cast [ ... x i8 ]* to i8*
    LLVMValueRef cast = LLVMConstGEP2(t_type->llvm_type(), glob, zeros, 2);
    return new ConstValue(string_type, cast);
  }
};

LLVMValueRef gen_num_num_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               NumType *lhs_nt, NumType *rhs_nt) {
  if (lhs_nt->bits > rhs_nt->bits)
    R = cast(R, rhs_nt, lhs_nt);
  else if (rhs_nt->bits > lhs_nt->bits)
    L = cast(L, lhs_nt, rhs_nt);
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
      error("Error: invalid float_float binary operator '" + token_to_str(op) +
            "'");
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
    num = LLVMBuildSub(
        curr_builder,
        LLVMConstInt((new NumType(32, false, false))->llvm_type(), 0, false),
        num, UN);
    /* falls through */
  case '+':
    return LLVMBuildGEP2(curr_builder, ptr_t->points_to->llvm_type(), ptr, &num,
                         1, "ptraddtmp");
  default:
    error("invalid ptr_num binary operator '" + token_to_str(op) + "'");
  }
}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST *LHS, *RHS;

public:
  BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS) {
    if (op_eq_ops.count(op)) {
      // if the op is an assignment operator (like +=, -=, *=), then transform
      // this binaryexpr into an assignment, and the RHS into the operator
      // part. (basically transforms a+=1 into a=a+1)
      RHS = new BinaryExprAST(op_eq_ops[op], LHS, RHS);
      op = '=';
    }
    this->op = op;
    this->LHS = LHS;
    this->RHS = RHS;
  }

  Type *get_type() {
    Type *lhs_t = LHS->get_type();
    Type *rhs_t = RHS->get_type();
    TypeType lhs_tt = lhs_t->type_type();
    TypeType rhs_tt = rhs_t->type_type();

    if (op == '=')
      return rhs_t;
    else if (lhs_tt == TypeType::Number && rhs_tt == TypeType::Number)
      return (binop_precedence[op] ==
              comparison_prec /* binop is a comparison */)
                 ? new NumType(1, false, false) // int < int returns bool
                 : lhs_t;                       // int + int returns int
    else if (lhs_tt == TypeType::Pointer &&
             rhs_tt == TypeType::Number) // ptr + int returns offsetted ptr
      return /* ptr */ lhs_t;
    else if (lhs_tt == TypeType::Number &&
             rhs_tt == TypeType::Pointer) // int + ptr returns offsetted ptr
      return /* ptr */ rhs_t;
    else
      error("Unknown ptr_ptr op");
  }

  Value *gen_assign() {
    LLVMValueRef store_ptr = LHS->gen_value()->gen_ptr();
    CastValue *val = RHS->gen_value()->cast_to(LHS->get_type());
    LLVMBuildStore(curr_builder, val->gen_val(), store_ptr);
    return new BasicLoadValue(store_ptr, get_type());
  }
  Value *gen_value() {
    if (op == '=')
      return gen_assign();
    Type *type = get_type();
    Type *lhs_t = LHS->get_type();
    Type *rhs_t = RHS->get_type();
    NumType *lhs_nt = dynamic_cast<NumType *>(lhs_t);
    NumType *rhs_nt = dynamic_cast<NumType *>(rhs_t);
    PointerType *lhs_pt = dynamic_cast<PointerType *>(lhs_t);
    PointerType *rhs_pt = dynamic_cast<PointerType *>(rhs_t);
    LLVMValueRef L = LHS->gen_value()->gen_val();
    LLVMValueRef R = RHS->gen_value()->gen_val();
    if (lhs_nt && rhs_nt)
      return new ConstValue(type, gen_num_num_binop(op, L, R, lhs_nt, rhs_nt));
    else if (lhs_nt && rhs_pt)
      return new ConstValue(type, gen_ptr_num_binop(op, R, L, rhs_pt, lhs_nt));
    else if (lhs_pt && rhs_nt)
      return new ConstValue(type, gen_ptr_num_binop(op, L, R, lhs_pt, rhs_nt));
    error("Unknown ptr_ptr op");
  }
};
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  int op;
  ExprAST *operand;
  Type *type;

public:
  UnaryExprAST(int op, ExprAST *operand) : op(op), operand(operand) {
    if (op == '*')
      if (PointerType *opt = dynamic_cast<PointerType *>(operand->get_type()))
        type = opt->get_points_to();
      else
        error("* can't be used on a non-pointer type");
    else if (op == '&')
      type = operand->get_type()->ptr();
    else
      type = operand->get_type();
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    auto zero =
        LLVMConstInt((new NumType(32, false, false))->llvm_type(), 0, false);
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
      return new BasicLoadValue(val->gen_val(), type);
    case '&':
      return new ConstValue(type, val->gen_ptr());
    case T_RETURN:
      LLVMBuildRet(curr_builder,
                   val->cast_to(curr_func_type->return_type)->gen_val());
      return val;
    default:
      error("invalid prefix unary operator '" + token_to_str(op) + "'");
    }
  }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
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
  CallExprAST(ExprAST *called, std::vector<ExprAST *> args)
      : called(called), args(args) {}

  Type *get_type() {
    FunctionType *func_t = get_func_type();
    if (func_t->vararg ? args.size() < func_t->arguments.size()
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
    return new ConstValue(func_t->return_type,
                          LLVMBuildCall2(curr_builder, func_t->llvm_type(),
                                         func, arg_vs, args.size(), UN));
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
            tt_to_str(base_type->type_type()));
  }

  Value *gen_value() {
    Type *type = get_type();
    LLVMValueRef index_v = index->gen_value()->gen_val();
    return new BasicLoadValue(LLVMBuildGEP2(curr_builder, type->llvm_type(),
                                            value->gen_value()->gen_val(),
                                            &index_v, 1, "indextmp"),
                              type);
  }
};

/// NumAccessExprAST - Expression class for accessing indexes on Tuples (a.0).
class NumAccessExprAST : public ExprAST {
  bool is_ptr;
  TupleType *source_type;
  Type *type = nullptr;

public:
  unsigned int index;
  ExprAST *source;
  NumAccessExprAST(unsigned int index, ExprAST *source)
      : index(index), source(source) {}
  void init() {
    if (type)
      return; // already initialized
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    }
    source_type = dynamic_cast<TupleType *>(st);
    type = source_type->get_elem_type(index);
  }

  Type *get_type() {
    init();
    return type;
  }

  Value *gen_value() {
    init();
    Value *src = source->gen_value();
    // If src is a struct-pointer (*String) then access on the value, if src
    // is a struct-value (String) then access on the pointer to where it's
    // stored.
    LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
    return new BasicLoadValue(LLVMBuildStructGEP2(curr_builder,
                                                  source_type->llvm_type(),
                                                  struct_ptr, index, UN),
                              type);
  }
};

/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  bool is_ptr;
  StructType *source_type;
  unsigned int index;
  Type *type = nullptr;

public:
  std::string key;
  ExprAST *source;
  PropAccessExprAST(std::string key, ExprAST *source)
      : key(key), source(source) {}
  void init() {
    if (type)
      return; // already initialized
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    }
    StructType *struct_t = dynamic_cast<StructType *>(st);
    if (!struct_t)
      error("Invalid property access for key '" + key + "', " +
            st->stringify() + " is not a struct.");
    index = struct_t->get_index(key);
    type = struct_t->get_elem_type(index);
    source_type = struct_t;
  }

  Type *get_type() {
    init();
    return type;
  }

  Value *gen_value() {
    init();
    Value *src = source->gen_value();
    // If src is a struct-pointer (*String) then access on the value, if src
    // is a struct-value (String) then access on the pointer to where it's
    // stored.
    LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
    return new BasicLoadValue(LLVMBuildStructGEP2(curr_builder,
                                                  source_type->llvm_type(),
                                                  struct_ptr, index, UN),
                              type);
  }
};

/// MethodCallExprAST - Expression class for calling methods (a.len()).
class MethodCallExprAST : public ExprAST {
  CallExprAST *underlying_call = nullptr;
  CallExprAST *get_call() {
    if (underlying_call)
      return underlying_call;
    ValueAndType *extension =
        curr_extension_methods[{source->get_type(), name}];
    bool ptr = !extension;
    if (ptr)
      extension = curr_extension_methods[{source->get_type()->ptr(), name}];
    if (extension) {
      args.push_back(ptr ? new UnaryExprAST('&', source) : source);
      return underlying_call = new CallExprAST(extension, args);
    } else {
      // call stored pointer to function
      auto prop = new PropAccessExprAST(name, source);
      return underlying_call = new CallExprAST(prop, args);
    }
  }

public:
  std::string name;
  ExprAST *source;
  std::vector<ExprAST *> args;
  MethodCallExprAST(std::string name, ExprAST *source,
                    std::vector<ExprAST *> args)
      : name(name), source(source), args(args) {}

  Type *get_type() { return get_call()->get_type(); }
  Value *gen_value() { return get_call()->gen_value(); }
};

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {

public:
  TypeAST *s_type;
  std::vector<std::pair<std::string, ExprAST *>> fields;
  NewExprAST(TypeAST *s_type,
             std::vector<std::pair<std::string, ExprAST *>> fields)
      : s_type(s_type), fields(fields) {}
  Type *get_type() { return s_type->type()->ptr(); }

  Value *gen_value() {
    StructType *st = dynamic_cast<StructType *>(s_type->type());
    if (!st)
      error("Cannot create instance of non-struct type " +
            s_type->type()->stringify());
    LLVMValueRef ptr = LLVMBuildMalloc(curr_builder, st->llvm_type(), "malloc");
    for (size_t i = 0; i < fields.size(); i++) {
      auto &[key, value] = fields[i];
      LLVMValueRef set_ptr = LLVMBuildStructGEP2(
          curr_builder, st->llvm_type(), ptr, st->get_index(key), key.c_str());
      LLVMBuildStore(
          curr_builder,
          value->gen_value()->cast_to(st->get_elem_type(i))->gen_val(),
          set_ptr);
    }
    return new ConstValue(st->ptr(), ptr);
  }
};

class TupleExprAST : public ExprAST {
  std::vector<ExprAST *> values;
  TupleType *t_type;
  Type *_type = nullptr;

public:
  bool is_new;
  TupleExprAST(std::vector<ExprAST *> values) : values(values) {}

  Type *get_type() {
    if (_type)
      return _type;
    std::vector<Type *> types;
    for (auto &value : values)
      types.push_back(value->get_type());
    t_type = new TupleType(types);
    if (is_new)
      return _type = t_type->ptr();
    else
      return _type = t_type;
  }

  Value *gen_value() {
    Type *type = get_type();
    LLVMValueRef ptr = (is_new ? LLVMBuildMalloc : LLVMBuildAlloca)(
        curr_builder, t_type->llvm_type(), is_new ? "malloc" : "alloca");
    for (size_t i = 0; i < values.size(); i++) {
      LLVMValueRef set_ptr = LLVMBuildStructGEP2(
          curr_builder, t_type->llvm_type(), ptr, i, "tupleset");
      LLVMBuildStore(curr_builder, values[i]->gen_value()->gen_val(), set_ptr);
    }
    if (is_new)
      return new ConstValue(type, ptr);
    else
      return new BasicLoadValue(ptr, type);
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
    // initialize previous exprs
    for (size_t i = 0; i < exprs.size() - 1; i++)
      exprs[i]->get_type();
    return exprs.back()->get_type();
  }
  Value *gen_value() {
    // generate code for all exprs and only return last expr
    for (size_t i = 0; i < exprs.size() - 1; i++)
      exprs[i]->gen_value();
    return exprs.back()->gen_value();
  }
};

/// NullExprAST - null
class NullExprAST : public ExprAST {
public:
  Type *type;
  NullExprAST(Type *type) : type(type) {}
  Type *get_type() { return type; }
  Value *gen_value() {
    return new ConstValue(type, LLVMConstNull(type->llvm_type()));
  }
};

class TypeAssertExprAST : public ExprAST {
public:
  TypeAST *a;
  TypeAST *b;
  TypeAssertExprAST(TypeAST *a, TypeAST *b) : a(a), b(b) {}
  Type *get_type() { return new VoidType(); }
  Value *gen_value() {
    if (a->eq(b))
      return NullExprAST(get_type()).gen_value();
    else
      error("Type mismatch in Type assertion, " + a->stringify() +
            " != " + b->stringify());
  }
};

class TypeDumpExprAST : public ExprAST {
public:
  TypeAST *type;
  TypeDumpExprAST(TypeAST *type) : type(type) {}
  Type *get_type() { return new VoidType(); }
  Value *gen_value() {
    std::cout << "[DUMP] Dumped type: " << type->stringify();
    return NullExprAST(get_type()).gen_value();
  }
};

/// TypeIfExprAST - Expression class for ifs based on type.
class TypeIfExprAST : public ExprAST {
  ExprAST *ret_expr = nullptr;
  ExprAST *pick() {
    if (ret_expr)
      return ret_expr;
    return ret_expr = a->type()->eq(b->type()) ? then : elze;
  }

public:
  ExprAST *then, *elze;
  TypeAST *a, *b;
  TypeIfExprAST(TypeAST *a, TypeAST *b, ExprAST *then,
                // elze because else cant be a variable name lol
                ExprAST *elze)
      : a(a), b(b), then(then), elze(elze) {}

  Type *get_type() { return pick()->get_type(); }
  Value *gen_value() { return pick()->gen_value(); }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
  ExprAST *cond, *then, *elze;
  Type *type;
  void init() {
    Type *then_t = then->get_type();
    type = then_t;
    if (elze == nullptr)
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
      : cond(cond), then(then), elze(elze) {}

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
    return new PHIValue(then_bb, then_v, else_bb, else_v);
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
    return new PHIValue(then_bb, then_v, else_bb, else_v);
  }
};

class ForExprAST : public IfExprAST {
  ExprAST *init;
  ExprAST *post;

public:
  ForExprAST(ExprAST *init, ExprAST *cond, ExprAST *body, ExprAST *post,
             ExprAST *elze)
      : init(init), post(post), IfExprAST(cond, body, elze) {}
  Value *gen_value() {
    IfExprAST::init();
    init->gen_value(); // let i = 0

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
    return new PHIValue(then_bb, then_v, else_bb, else_v);
  }
};

class StructAST : public TopLevelAST {
  StructTypeAST type;

public:
  StructAST(std::string name,
            std::vector<std::pair<std::string, TypeAST *>> members)
      : type(name, members) {}
  LLVMValueRef gen_toplevel() {
    curr_named_types[type.name] = type.type();
    return nullptr;
  }
};

class TypeDefAST : public TopLevelAST {
  std::string name;
  TypeAST *type;

public:
  TypeDefAST(std::string name, TypeAST *type) : name(name), type(type) {}
  LLVMValueRef gen_toplevel() {
    curr_named_types[name] = type->type();
    return nullptr;
  }
};

/// DeclareExprAST - Expression class for defining an declare.
class DeclareExprAST : public TopLevelAST {
  LetExprAST *let = nullptr;
  PrototypeAST *prot = nullptr;
  Type *type;

public:
  DeclareExprAST(LetExprAST *let) : let(let) {
    type = let->get_type();
    curr_named_variables[let->id] = new ValueAndType(type);
  }
  DeclareExprAST(PrototypeAST *prot) : prot(prot) {
    type = prot->get_type();
    curr_named_variables[prot->name] = new ValueAndType(type);
  }
  LLVMValueRef gen_toplevel() {
    if (let)
      return let->gen_declare();
    else
      return prot->codegen();
  }
};