#pragma once
#include "types.cpp"
#include "utils.cpp"
#include "values.cpp"

// Includes function arguments
static std::map<std::string, Value *> curr_named_variables;
static std::map<std::string, Type *> curr_named_var_types;
static std::map<std::string, Type *> curr_named_types;

class TopLevelAST {
public:
  virtual LLVMValueRef gen_toplevel() = 0;
};
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Type *get_type() = 0;
  virtual Value *gen_value() = 0;
};

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
    fprintf(stderr, "Error: Invalid number type id '%c'", type_char);
    exit(1);
  }
}
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  const char *val;
  size_t val_len;
  unsigned int base;
  NumType *type;

public:
  NumberExprAST(char *val, size_t val_len, char type_char, bool has_dot,
                unsigned int base)
      : val(val), val_len(val_len), base(base) {
    type = num_char_to_type(type_char, has_dot);
  }
  NumberExprAST(unsigned int val, char type_char) : base(10) {
    type = num_char_to_type(type_char, false);
    this->val = num_to_str(val);
    this->val_len = log10(val) + 1;
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    LLVMValueRef num;
    if (type->is_floating)
      if (base != 10) {
        error("floating-point numbers with a base that isn't decimal aren't "
              "supported.");
      } else
        num = LLVMConstRealOfStringAndSize(type->llvm_type(), val, val_len);
    else
      num = LLVMConstIntOfStringAndSize(type->llvm_type(), val, val_len, base);
    return new ConstValue(type, num);
  }
};
/// BoolExprAST - Expression class for boolean literals (true or false).
class BoolExprAST : public ExprAST {
  bool value;
  Type *type;

public:
  BoolExprAST(bool value) : value(value) {
    type = new NumType(1, false, false);
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    return new ConstValue(type, value ? LLVMConstAllOnes(type->llvm_type())
                                      : LLVMConstNull(type->llvm_type()));
  }
};

class CastExprAST : public ExprAST {
  ExprAST *value;
  Type *from;
  Type *to;

public:
  CastExprAST(ExprAST *value, Type *to) : value(value), to(to) {
    from = value->get_type();
  }
  Value *gen_value() { return value->gen_value()->cast_to(to); }
  Type *get_type() { return to; }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  char *name;
  size_t name_len;
  Type *type;

public:
  VariableExprAST(char *name, size_t name_len)
      : name(name), name_len(name_len) {
    type = curr_named_var_types[name];
    if (!type) {
      fprintf(stderr, "Error: Variable '%s' doesn't exist.", name);
      exit(1);
    }
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    return curr_named_variables[std::string(name, name_len)];
  }
};

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST, public TopLevelAST {

public:
  char *id;
  size_t id_len;
  Type *type;
  ExprAST *value;
  bool constant;
  bool global;
  LetExprAST(char *id, size_t id_len, Type *type, ExprAST *value, bool constant,
             bool global)
      : id(id), id_len(id_len), constant(constant), global(global) {
    if (type)
      curr_named_var_types[std::string(id, id_len)] = type;
    else if (value != nullptr)
      curr_named_var_types[std::string(id, id_len)] = type = value->get_type();
    else
      error("Untyped valueless variable");
    this->type = type;
    this->value = value;
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_toplevel() {
    LLVMValueRef ptr = LLVMAddGlobal(curr_module, type->llvm_type(), id);
    if (value) {
      Value *val = value->gen_value();
      if (ConstValue *expr = dynamic_cast<ConstValue *>(val))
        LLVMSetInitializer(ptr, val->gen_val());
      else
        error("Global variable needs a constant value inside it");
    }
    curr_named_variables[std::string(id, id_len)] =
        new BasicLoadValue(ptr, type);
    return ptr;
  }
  Value *gen_value() {
    if (constant) {
      if (value)
        return curr_named_variables[std::string(id, id_len)] =
                   new NamedValue(value->gen_value(), id, id_len);
      else
        error("Constant variables need an initialization value");
    }
    LLVMValueRef ptr = LLVMBuildAlloca(curr_builder, type->llvm_type(), id);
    LLVMSetValueName2(ptr, id, id_len);
    if (value) {
      LLVMValueRef llvm_val = value->gen_value()->cast_to(type)->gen_val();
      LLVMBuildStore(curr_builder, llvm_val, ptr);
    }
    return curr_named_variables[std::string(id, id_len)] =
               new BasicLoadValue(ptr, type);
  }
  LLVMValueRef gen_declare() {
    LLVMValueRef global = LLVMAddGlobal(curr_module, type->llvm_type(), id);
    curr_named_variables[std::string(id, id_len)] =
        new BasicLoadValue(global, type);
    return global;
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

/// StringExprAST - Expression class for multiple chars ("hello")
class StringExprAST : public ExprAST {
  char *chars;
  size_t length;
  ArrayType *t_type;
  PointerType *p_type;

public:
  StringExprAST(char *chars, size_t length) : chars(chars), length(length) {
    if (chars[length - 1] != '\0')
      error("C-style strings should be fed into StringExprAST including the "
            "last null-byte");
    if (!char_type)
      char_type = new NumType(8, false, false);
    t_type = new ArrayType(char_type, length);
    p_type = new PointerType(char_type);
  }
  Type *get_type() { return p_type; }
  Value *gen_value() {
    LLVMValueRef str = LLVMConstString(chars, length, true);
    LLVMValueRef glob = LLVMAddGlobal(curr_module, t_type->llvm_type(), ".str");
    LLVMSetInitializer(glob, str);
    LLVMSetLinkage(glob, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(glob, LLVMGlobalUnnamedAddr);
    LLVMValueRef zeros[2] = {
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false),
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false)};
    // cast [ ... x i8 ]* to i8*
    LLVMValueRef cast = LLVMConstGEP2(t_type->llvm_type(), glob, zeros, 2);
    return new ConstValue(p_type, cast);
  }
  // LLVMValueRef u_gen_load() {
  //   return gen_variable((char *)".str", false)->gen_load();
  // }
  // Value *gen_variable(char *name, bool constant) {
  //   LLVMValueRef str = LLVMConstString(chars, length, true);
  //   LLVMValueRef glob = LLVMAddGlobal(curr_module, type->llvm_type(), name);
  //   LLVMSetInitializer(glob, str);
  //   LLVMSetGlobalConstant(glob, constant);
  //   return new BasicLoadValue(glob, type);
  // }
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
      fprintf(stderr, "Error: invalid float_float binary operator '%c'", op);
      exit(1);
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
      fprintf(stderr, "Error: invalid int_int binary operator '%c'", op);
      exit(1);
    }
  } else {
    fprintf(stderr, "Error: invalid float_int binary operator '%c'", op);
    exit(1);
  }
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
    fprintf(stderr, "Error: invalid ptr_num binary operator '%c'", op);
    exit(1);
  }
}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST *LHS, *RHS;
  Type *type;

public:
  BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS) {
    if (op_eq_ops.count(op)) {
      // if the op is an assignment operator (like +=, -=, *=), then transform
      // this binaryexpr into an assignment, and the RHS into the operator part.
      // (basically transforms a+=1 into a=a+1)
      RHS = new BinaryExprAST(op_eq_ops[op], LHS, RHS);
      op = '=';
    }
    this->op = op;
    this->LHS = LHS;
    this->RHS = RHS;
    Type *lhs_t = LHS->get_type();
    Type *rhs_t = RHS->get_type();
    TypeType lhs_tt = lhs_t->type_type();
    TypeType rhs_tt = rhs_t->type_type();

    if (op == '=')
      type = rhs_t;
    else if (lhs_tt == TypeType::Number && rhs_tt == TypeType::Number)
      type =
          (binop_precedence[op] == comparison_prec /* binop is a comparison */)
              ? new NumType(1, false, false) // int < int returns uint1 (bool)
              : /* todo get max size and return that type */ lhs_t; // int + int
                                                                    // returns
                                                                    // int
    else if (lhs_tt == TypeType::Pointer &&
             rhs_tt == TypeType::Number) // ptr + int returns offsetted ptr
      type = /* ptr */ lhs_t;
    else if (lhs_tt == TypeType::Number &&
             rhs_tt == TypeType::Pointer) // int + ptr returns offsetted ptr
      type = /* ptr */ rhs_t;
    else
      error("Unknown ptr_ptr op");
  }

  Type *get_type() { return type; }

  Value *gen_assign() {
    LLVMValueRef store_ptr = LHS->gen_value()->gen_ptr();
    Value *val = RHS->gen_value()->cast_to(LHS->get_type());
    LLVMBuildStore(curr_builder, val->gen_val(), store_ptr);
    return new BasicLoadValue(store_ptr, type);
  }
  Value *gen_value() {
    Type *lhs_t = LHS->get_type();
    PointerType *lhs_pt = dynamic_cast<PointerType *>(lhs_t);
    if (op == '=')
      return gen_assign();
    Type *rhs_t = RHS->get_type();
    NumType *lhs_nt = dynamic_cast<NumType *>(lhs_t);
    NumType *rhs_nt = dynamic_cast<NumType *>(rhs_t);
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
      type = new PointerType(operand->get_type());
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
      LLVMBuildRet(curr_builder, val->gen_val());
      return val;
    default:
      fprintf(stderr, "Error: invalid prefix unary operator '%c'", op);
      exit(1);
    }
  }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  FunctionType *func_t;
  ExprAST *called;
  ExprAST **args;
  size_t args_len;
  bool is_ptr;
  Type *type;

public:
  CallExprAST(ExprAST *called, ExprAST **args, size_t args_len)
      : called(called), args(args), args_len(args_len) {
    func_t = dynamic_cast<FunctionType *>(called->get_type());
    if (!func_t) {
      if (PointerType *ptr = dynamic_cast<PointerType *>(called->get_type()))
        func_t = dynamic_cast<FunctionType *>(ptr->get_points_to());
      else {
        fprintf(stderr, "Error: Function doesn't exist or is not a function");
        exit(1);
      }
    }
    if (func_t->vararg ? args_len < func_t->arg_count
                       : args_len != func_t->arg_count) {
      fprintf(stderr,
              "Error: Incorrect # arguments passed. (Expected %d, got %d)",
              func_t->arg_count, (unsigned int)args_len);
      exit(1);
    }

    type = func_t->return_type;
  }

  Type *get_type() { return type; }
  Value *gen_value() {
    LLVMValueRef func = called->gen_value()->gen_val();
    if (!func)
      error("Unknown function referenced");
    LLVMValueRef *arg_vs = alloc_arr<LLVMValueRef>(args_len);
    for (unsigned i = 0; i < args_len; i++) {
      if (i < func_t->arg_count)
        arg_vs[i] =
            args[i]->gen_value()->cast_to(func_t->arguments[i])->gen_val();
      else
        arg_vs[i] = args[i]->gen_value()->gen_val();
    }
    return new ConstValue(func_t->return_type,
                          LLVMBuildCall2(curr_builder, func_t->llvm_type(),
                                         func, arg_vs, args_len, UN));
  }
};

/// IndexExprAST - Expression class for accessing indexes (a[0]).
class IndexExprAST : public ExprAST {
  ExprAST *value;
  ExprAST *index;
  Type *type;

public:
  IndexExprAST(ExprAST *value, ExprAST *index) : value(value), index(index) {
    Type *base_type = value->get_type();
    if (PointerType *p_type = dynamic_cast<PointerType *>(base_type))
      type = p_type->get_points_to();
    else if (ArrayType *arr_type = dynamic_cast<ArrayType *>(base_type))
      type = arr_type->get_elem_type();
    else {
      fprintf(stderr,
              "Invalid index, type not arrayish.\n"
              "Expected: array | pointer \nGot: %s",
              tt_to_str(base_type->type_type()));
      exit(1);
    }
  }

  Type *get_type() { return type; }

  Value *gen_value() {
    LLVMValueRef index_v = index->gen_value()->gen_val();
    return new BasicLoadValue(LLVMBuildGEP2(curr_builder, type->llvm_type(),
                                            value->gen_value()->gen_val(),
                                            &index_v, 1, "indextmp"),
                              type);
  }
};

/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  ExprAST *source;
  bool is_ptr;
  TupleType *source_type;
  unsigned int index;
  char *key;
  Type *type;

public:
  PropAccessExprAST(char *key, size_t name_len, ExprAST *source)
      : key(key), source(source) {
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    }
    StructType *struct_t = dynamic_cast<StructType *>(st);
    index = struct_t->get_index(key, name_len);
    type = struct_t->get_elem_type(index);
    source_type = struct_t;
  }
  PropAccessExprAST(unsigned int idx, ExprAST *source)
      : index(idx), source(source) {
    Type *st = source->get_type();
    if (st->type_type() == TypeType::Pointer) {
      st = dynamic_cast<PointerType *>(source->get_type())->get_points_to();
      is_ptr = true;
    }
    source_type = dynamic_cast<TupleType *>(st);
    type = source_type->get_elem_type(index);
  }

  Type *get_type() { return type; }

  Value *gen_value() {
    Value *src = source->gen_value();
    // If src is a struct-pointer (*String) then access on the value, if src is
    // a struct-value (String) then access on the pointer to where it's stored.
    LLVMValueRef struct_ptr = is_ptr ? src->gen_val() : src->gen_ptr();
    return new BasicLoadValue(LLVMBuildStructGEP2(curr_builder,
                                                  source_type->llvm_type(),
                                                  struct_ptr, index, UN),
                              type);
  }
};

struct CompleteExtensionName {
  char *str;
  size_t len;
};
CompleteExtensionName get_complete_extension_name(Type *base_type, char *name,
                                                  size_t name_len) {
  char *called_type = LLVMPrintTypeToString(base_type->llvm_type());
  CompleteExtensionName cen;
  cen.len = 2 + strlen(called_type) + name_len;
  cen.str = alloc_c(cen.len);
  strcat(cen.str, called_type);
  strcat(cen.str, "::");
  strcat(cen.str, name);
  return cen;
}
/// MethodCallExprAST - Expression class for calling methods (a.len()).
class MethodCallExprAST : public ExprAST {
  CallExprAST *underlying_call;

public:
  MethodCallExprAST(char *name, size_t name_len, ExprAST *source,
                    ExprAST **args, size_t args_len) {
    CompleteExtensionName cen =
        get_complete_extension_name(source->get_type(), name, name_len);
    if (curr_named_var_types[std::string(cen.str, cen.len)]) {
      VariableExprAST *called_function = new VariableExprAST(cen.str, cen.len);
      ExprAST **args_with_this = realloc_arr<ExprAST *>(args, args_len + 1);
      args_with_this[args_len] = source;
      underlying_call =
          new CallExprAST(called_function, args_with_this, args_len + 1);
    } else {
      // call stored pointer to function
      auto prop = new PropAccessExprAST(name, name_len, source);
      underlying_call = new CallExprAST(prop, args, args_len);
    }
  }

  Type *get_type() { return underlying_call->get_type(); }
  Value *gen_value() { return underlying_call->gen_value(); }
};

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {
  StructType *s_type;
  PointerType *p_type;
  unsigned int *indexes;
  char **keys;
  ExprAST **values;
  unsigned int key_count;

public:
  NewExprAST(StructType *s_type, char **keys, size_t *key_lens,
             ExprAST **values, unsigned int key_count)
      : s_type(s_type), values(values), key_count(key_count) {
    indexes = alloc_arr<unsigned int>(key_count);
    for (unsigned int i = 0; i < key_count; i++)
      indexes[i] = s_type->get_index(keys[i], key_lens[i]);
    p_type = new PointerType(s_type);
  }

  Type *get_type() { return p_type; }

  Value *gen_value() {
    LLVMValueRef ptr =
        LLVMBuildMalloc(curr_builder, s_type->llvm_type(), "malloc");
    for (unsigned int i = 0; i < key_count; i++) {
      LLVMValueRef llvm_indexes[2] = {
          LLVMConstInt(LLVMInt32Type(), 0, false),
          LLVMConstInt(LLVMInt32Type(), indexes[i], false)};
      LLVMValueRef set_ptr = LLVMBuildStructGEP2(
          curr_builder, s_type->llvm_type(), ptr, indexes[i], "tmpgep");
      LLVMBuildStore(curr_builder, values[i]->gen_value()->gen_val(), set_ptr);
    }
    return new ConstValue(p_type, ptr);
  }
};

class TupleExprAST : public ExprAST {
  TupleType *type;
  PointerType *p_type;
  ExprAST **values;
  size_t length;

public:
  bool use_malloc;
  TupleExprAST(ExprAST **values, size_t length)
      : values(values), length(length) {
    Type **types = alloc_arr<Type *>(length);
    for (size_t i = 0; i < length; i++)
      types[i] = values[i]->get_type();
    type = new TupleType(types, length);
    p_type = new PointerType(type);
  }

  Type *get_type() { return p_type; }

  Value *gen_value() {
    LLVMValueRef ptr = (use_malloc ? LLVMBuildMalloc : LLVMBuildAlloca)(
        curr_builder, type->llvm_type(), use_malloc ? "malloc" : "alloca");
    for (size_t i = 0; i < length; i++) {
      LLVMValueRef llvm_indexes[2] = {LLVMConstInt(LLVMInt32Type(), 0, false),
                                      LLVMConstInt(LLVMInt32Type(), i, false)};
      LLVMValueRef set_ptr = LLVMBuildStructGEP2(
          curr_builder, type->llvm_type(), ptr, i, "tmpgep");
      LLVMBuildStore(curr_builder, values[i]->gen_value()->gen_val(), set_ptr);
    }
    return new ConstValue(p_type, ptr);
  }
};

class BlockExprAST : public ExprAST {
  ExprAST **exprs;
  size_t exprs_len;
  Type *type;

public:
  BlockExprAST(ExprAST **exprs, size_t exprs_len)
      : exprs(exprs), exprs_len(exprs_len) {
    if (exprs_len == 0)
      error("block can't be empty.");
    type = exprs[exprs_len - 1]->get_type();
  }
  Type *get_type() { return type; }
  Value *gen_value() {
    // generate code for all exprs and only return last expr
    for (unsigned int i = 0; i < exprs_len - 1; i++)
      exprs[i]->gen_value();
    return exprs[exprs_len - 1]->gen_value();
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

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
  ExprAST *cond, *then, *elze;
  Type *type;
  IfExprAST(ExprAST *cond, ExprAST *then,
            // elze because else cant be a variable name lol
            ExprAST *elze)
      : cond(cond), then(then) {
    Type *then_t = then->get_type();
    type = then_t;
    if (elze == nullptr)
      elze = new NullExprAST(type);
    Type *else_t = elze->get_type();
    if (then_t->neq(else_t)) {
      fprintf(
          stderr,
          "Error: conditional's then and else side don't have the same type, ");
      then_t->log_diff(else_t);
      fprintf(stderr, ".\n");
      exit(1);
    }
    this->elze = elze;
  }

  Type *get_type() { return type; }

  Value *gen_value() {
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

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the
/// number of arguments the function takes).
class PrototypeAST {
public:
  char **arg_names;
  size_t *arg_name_lengths;
  Type **arg_types;
  FunctionType *type;
  unsigned int arg_count;
  char *name;
  size_t name_len;
  PrototypeAST(char *name, size_t name_len, char **arg_names,
               size_t *arg_name_lengths, Type **arg_types,
               unsigned int arg_count, Type *return_type, bool vararg)
      : name(name), name_len(name_len), arg_names(arg_names),
        arg_name_lengths(arg_name_lengths), arg_types(arg_types),
        arg_count(arg_count) {
    for (unsigned i = 0; i != arg_count; ++i)
      curr_named_var_types[std::string(arg_names[i], arg_name_lengths[i])] =
          arg_types[i];
    curr_named_var_types[std::string(name, name_len)] = type =
        new FunctionType(return_type, arg_types, arg_count, vararg);
  }
  PrototypeAST(Type *this_type, char *name, size_t name_len, char **arg_names,
               size_t *arg_name_lengths, Type **arg_types,
               unsigned int arg_count, Type *return_type, bool vararg) {
    CompleteExtensionName cen =
        get_complete_extension_name(this_type, name, name_len);
    arg_count++;
    arg_names = realloc_arr<char *>(arg_names, arg_count);
    arg_names[arg_count - 1] = strdup("this");
    arg_name_lengths = realloc_arr<size_t>(arg_name_lengths, arg_count);
    arg_name_lengths[arg_count - 1] = 4;
    arg_types = realloc_arr<Type *>(arg_types, arg_count);
    arg_types[arg_count - 1] = this_type;
    new (this) PrototypeAST(cen.str, cen.len, arg_names, arg_name_lengths,
                            arg_types, arg_count, return_type, vararg);
  }
  FunctionType *get_type() { return type; }
  LLVMValueRef codegen() {
    LLVMValueRef func = LLVMAddFunction(curr_module, name, type->llvm_type());
    curr_named_variables[std::string(name, name_len)] =
        new FuncValue(type, func);
    // Set names for all arguments.
    LLVMValueRef *params = alloc_arr<LLVMValueRef>(arg_count);
    LLVMGetParams(func, params);
    for (unsigned i = 0; i != arg_count; ++i)
      LLVMSetValueName2(params[i], arg_names[i], arg_name_lengths[i]);

    LLVMSetValueName2(func, name, name_len);
    LLVMPositionBuilderAtEnd(curr_builder, LLVMGetFirstBasicBlock(func));
    return func;
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
    register_declare();
  }
  DeclareExprAST(PrototypeAST *prot) : prot(prot) {
    type = prot->get_type();
    register_declare();
  }
  void register_declare() {
    if (FunctionType *fun_t = dynamic_cast<FunctionType *>(type))
      curr_named_var_types[std::string(prot->name, prot->name_len)] = fun_t;
    else
      curr_named_var_types[std::string(let->id, let->id_len)] = type;
  }
  LLVMValueRef gen_toplevel() {
    register_declare();
    if (let)
      return let->gen_declare();
    else
      return prot->codegen();
  }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public TopLevelAST {
  ExprAST *body;

public:
  PrototypeAST *proto;
  FunctionAST(PrototypeAST *proto, ExprAST *body) {
    if (proto->type->return_type == nullptr)
      proto->type->return_type = body->get_type();
    this->proto = proto;
    this->body = body;
  }

  LLVMValueRef gen_toplevel() {
    // First, check for an existing function from a previous 'declare'
    // declaration.
    LLVMValueRef func = LLVMGetNamedFunction(curr_module, proto->name);

    if (!func)
      func = proto->codegen();

    if (!func)
      error("funcless behavior");

    if (LLVMCountBasicBlocks(func) != 0)
      error("Function cannot be redefined.");

    auto block = LLVMAppendBasicBlockInContext(curr_ctx, func, "");
    LLVMPositionBuilderAtEnd(curr_builder, block);
    size_t args_len = LLVMCountParams(func);
    LLVMValueRef *params = alloc_arr<LLVMValueRef>(args_len);
    LLVMGetParams(func, params);
    size_t unused = 0;
    for (unsigned i = 0; i != args_len; ++i)
      curr_named_variables[LLVMGetValueName2(params[i], &unused)] =
          new ConstValue(proto->arg_types[i], params[i]);
    Value *ret_val = body->gen_value()->cast_to(proto->type->return_type);
    // Finish off the function.
    LLVMBuildRet(curr_builder, ret_val->gen_val());
    unnamed_acc = 0;
    return func;
    // doesnt exist in c api (i think)
    // // Validate the generated code, checking for consistency.
    // // verifyFunction(*TheFunction);
  }
};

class StructAST : public TopLevelAST {
  char *name;
  size_t name_len;
  char **names;
  size_t *name_lengths;
  Type **types;
  unsigned int count;

public:
  StructAST(char *name, size_t name_len, char **names, size_t *name_lengths,
            Type **types, unsigned int count)
      : name(name), name_len(name_len), names(names),
        name_lengths(name_lengths), types(types), count(count) {}
  LLVMValueRef gen_toplevel() {
    curr_named_types[std::string(name, name_len)] =
        new StructType(name, name_len, names, name_lengths, types, count);
    return nullptr;
  }
};

class TypeDefAST : public TopLevelAST {
  char *name;
  size_t name_len;
  Type *type;

public:
  TypeDefAST(char *name, size_t name_len, Type *type)
      : name(name), name_len(name_len), type(type) {}
  LLVMValueRef gen_toplevel() {
    curr_named_types[std::string(name, name_len)] = type;
    return nullptr;
  }
};