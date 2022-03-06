#pragma once
#include "types.cpp"
#include "utils.cpp"
#include "variables.cpp"

// Includes function arguments
static std::map<std::string, Variable *> curr_named_variables;
static std::map<std::string, Type *> curr_named_var_types;
static std::map<std::string, StructType *> curr_named_structs;

class TopLevelAST {
public:
  virtual void gen_toplevel() = 0;
};
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Type *get_type() = 0;
  // Generate the value of gen_ptr
  virtual LLVMValueRef gen_val() = 0;
  // Generate a pointer to gen_val
  virtual LLVMValueRef gen_ptr() {
    LLVMValueRef ptr =
        LLVMBuildAlloca(curr_builder, get_type()->llvm_type(), "alloctmp");
    LLVMBuildStore(curr_builder, gen_val(), ptr);
    return ptr;
  }
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
    return nullptr;
  }
}
/// ConstantExpr - Base class for all const-able expression nodes.
class ConstantExpr {
public:
  virtual Variable *gen_variable(char *name, bool constant) = 0;
};
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST, public ConstantExpr {
  char *val;
  unsigned int val_len;
  unsigned int base;
  NumType *type;

public:
  NumberExprAST(char *val, unsigned int val_len, char type_char, bool has_dot,
                unsigned int base)
      : val(val), val_len(val_len), base(base) {
    type = num_char_to_type(type_char, has_dot);
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    if (type->is_floating)
      if (base != 10) {
        error("floating-point numbers with a base that isn't decimal aren't "
              "supported.");
        return nullptr;
      } else
        return LLVMConstRealOfStringAndSize(type->llvm_type(), val, val_len);
    else
      return LLVMConstIntOfStringAndSize(type->llvm_type(), val, val_len, base);
  }
  Variable *gen_variable(char *name, bool constant) {
    if (constant)
      return new ConstVariable(gen_val(), nullptr);
    LLVMValueRef val = gen_val();
    LLVMValueRef glob = LLVMAddGlobal(curr_module, type->llvm_type(), name);
    LLVMSetGlobalConstant(glob, false);
    LLVMSetInitializer(glob, val);
    return new BasicLoadVariable(glob, type);
  }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  char *name;
  unsigned int name_len;
  Type *type;

public:
  VariableExprAST(char *name, unsigned int name_len)
      : name(name), name_len(name_len) {
    type = curr_named_var_types[name];
    if (!type) {
      fprintf(stderr, "Error: Variable '%s' doesn't exist.", name);
      exit(1);
    }
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    Variable *v = curr_named_variables[std::string(name, name_len)];
    if (!v) {

      fprintf(stderr, "Error: Variable '%s' doesn't exist.", name);
      exit(1);
    }
    return v->gen_load();
  }
  LLVMValueRef gen_ptr() {
    Variable *v = curr_named_variables[std::string(name, name_len)];
    if (!v) {
      fprintf(stderr, "Error: Variable '%s' doesn't exist.", name);
      exit(1);
    }
    return v->gen_ptr();
  }
};

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST, public TopLevelAST {

public:
  char *id;
  unsigned int id_len;
  Type *type;
  ExprAST *value;
  bool constant;
  bool global;
  LetExprAST(char *id, unsigned int id_len, Type *type, ExprAST *value,
             bool constant, bool global)
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
  void gen_toplevel() {
    Variable *var;
    if (!value)
      var = new BasicLoadVariable(
          LLVMAddGlobal(curr_module, type->llvm_type(), id), type);
    else if (ConstantExpr *expr = dynamic_cast<ConstantExpr *>(value))
      var = expr->gen_variable(id, constant);
    else
      error("Global variable needs a constant value inside it");
    curr_named_variables[std::string(id, id_len)] = var;
  }
  LLVMValueRef gen_val() {
    if (constant) {
      if (value) {
        LLVMValueRef const_val = value->gen_val();
        curr_named_variables[std::string(id, id_len)] =
            new ConstVariable(const_val, nullptr);
        return const_val;
      } else
        error("Constant variables need an initialization value");
    }
    LLVMValueRef ptr = LLVMBuildAlloca(curr_builder, type->llvm_type(), id);
    curr_named_variables[std::string(id, id_len)] =
        new BasicLoadVariable(ptr, type);
    if (value) {
      LLVMValueRef llvm_val = value->gen_val();
      LLVMBuildStore(curr_builder, llvm_val, ptr);
      return llvm_val;
    } else
      return LLVMConstNull(type->llvm_type());
  }
  LLVMValueRef gen_ptr() {
    if (constant)
      error("Can't point to a constant");
    LLVMValueRef ptr = LLVMBuildAlloca(curr_builder, type->llvm_type(), id);
    curr_named_variables[std::string(id, id_len)] =
        new BasicLoadVariable(ptr, type);
    if (value) {
      LLVMValueRef llvm_val = value->gen_val();
      LLVMBuildStore(curr_builder, llvm_val, ptr);
    }
    return ptr;
  }
  LLVMValueRef gen_declare() {
    LLVMValueRef global = LLVMAddGlobal(curr_module, type->llvm_type(), id);
    curr_named_variables[std::string(id, id_len)] =
        new BasicLoadVariable(global, type);
    return global;
  }
};

/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public ExprAST {
  char charr;

public:
  CharExprAST(char charr) : charr(charr) {}
  Type *get_type() { return new NumType(8, false, false); }
  LLVMValueRef gen_val() { return LLVMConstInt(int_8_type, charr, false); }
};

/// StringExprAST - Expression class for multiple chars ("hello")
class StringExprAST : public ExprAST, public ConstantExpr {
  char *chars;
  unsigned int length;
  PointerType *type;
  ArrayType *array_type;

public:
  StringExprAST(char *chars, unsigned int length)
      : chars(chars), length(length) {
    if (chars[length - 1] != '\0')
      error("C-style strings should be fed into StringExprAST including the "
            "last null-byte");
    type = new PointerType(new NumType(8, false, false));
    array_type = new ArrayType(new NumType(8, false, false), length);
  }
  PointerType *get_type() { return type; }
  ArrayType *get_array_type() { return array_type; }
  LLVMValueRef gen_val() {
    return gen_variable((char *)".str", false)->gen_load();
  }
  Variable *gen_variable(char *name, bool constant) {
    LLVMValueRef str = LLVMConstString(chars, length, true);
    LLVMValueRef zeros[2] = {
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false),
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false)};
    LLVMValueRef glob =
        LLVMAddGlobal(curr_module, get_array_type()->llvm_type(), name);
    LLVMSetInitializer(glob, str);
    LLVMSetGlobalConstant(glob, constant);
    LLVMValueRef cast =
        LLVMConstGEP2(get_array_type()->llvm_type(), glob, zeros, 2);
    return new ConstVariable(cast, glob);
  }
};

LLVMValueRef gen_num_num_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               NumType *lhs_nt, NumType *rhs_nt) {
  bool floating = lhs_nt->is_floating && rhs_nt->is_floating;
  if (floating)
    switch (op) {
    case '+':
      return LLVMBuildFAdd(curr_builder, L, R, "faddtmp");
    case '-':
      return LLVMBuildFSub(curr_builder, L, R, "fsubtmp");
    case '*':
      return LLVMBuildFMul(curr_builder, L, R, "fmultmp");
    case T_LAND:
    case '&':
      return LLVMBuildAnd(curr_builder, L, R, "fandtmp");
    case T_LOR:
    case '|':
      return LLVMBuildOr(curr_builder, L, R, "fortmp");
    case '<':
      return LLVMBuildFCmp(curr_builder, LLVMRealULT, L, R, "fcmptmp");
    case '>':
      return LLVMBuildFCmp(curr_builder, LLVMRealUGT, L, R, "fcmptmp");
    case T_LEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealULE, L, R, "icmptmp");
    case T_GEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUGE, L, R, "icmptmp");
    case T_EQEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUEQ, L, R, "icmptmp");
    case T_NEQ:
      return LLVMBuildFCmp(curr_builder, LLVMRealUNE, L, R, "icmptmp");
    default:
      fprintf(stderr, "Error: invalid float_float binary operator '%c'", op);
      exit(1);
    }
  else if (!lhs_nt->is_floating && !rhs_nt->is_floating) {
    bool is_signed = lhs_nt->is_signed && rhs_nt->is_signed;
    switch (op) {
    case '+':
      return LLVMBuildAdd(curr_builder, L, R, "iaddtmp");
    case '-':
      return LLVMBuildSub(curr_builder, L, R, "isubtmp");
    case '*':
      return LLVMBuildMul(curr_builder, L, R, "imultmp");
    case T_LAND:
    case '&':
      return LLVMBuildAnd(curr_builder, L, R, "fandtmp");
    case T_LOR:
    case '|':
      return LLVMBuildOr(curr_builder, L, R, "fortmp");
    case '<':
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSLT : LLVMIntULT, L,
                           R, "icmptmp");
    case '>':
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSGT : LLVMIntUGT, L,
                           R, "icmptmp");
    case T_LEQ:
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSLE : LLVMIntULE, L,
                           R, "icmptmp");
    case T_GEQ:
      return LLVMBuildICmp(curr_builder, is_signed ? LLVMIntSGE : LLVMIntUGE, L,
                           R, "icmptmp");
    case T_EQEQ:
      return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntEQ, L, R,
                           "icmptmp");
    case T_NEQ:
      return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntNE, L, R,
                           "icmptmp");
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
        num, "ptrmintmp");
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
  char op;
  ExprAST *LHS, *RHS;
  Type *type;

public:
  BinaryExprAST(char op, ExprAST *LHS, ExprAST *RHS)
      : op(op), LHS(LHS), RHS(RHS) {
    Type *lhs_t = LHS->get_type();
    Type *rhs_t = RHS->get_type();
    TypeType lhs_tt = lhs_t->type_type();
    TypeType rhs_tt = rhs_t->type_type();

    if (op == '=') {
      // if the LHS is a variable, ptr is implied.
      if (dynamic_cast<VariableExprAST *>(LHS) ||
          lhs_t->eq(new PointerType(rhs_t)))
        type = rhs_t;
      else {
        fprintf(stderr, "Invalid variable assignment, unequal types.");
        lhs_t->log_diff(rhs_t);
        exit(1);
      }
    } else if (lhs_tt == TypeType::Number &&
               rhs_tt == TypeType::Number) // int + int returns int, int < int
                                           // returns int1 (bool)
      type = (binop_precedence[op] == 10 /* comparison */)
                 ? new NumType(1, false, false)
                 : /* todo get max size and return that type */ lhs_t;
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

  LLVMValueRef gen_assign(bool ptr) {
    LLVMValueRef set_to;
    if (VariableExprAST *left_var = dynamic_cast<VariableExprAST *>(LHS))
      // for 'a = 3'. If you want to override this behavior (and set the pointer
      // referenced in a) use 'a+0 = 3'
      set_to = left_var->gen_ptr();
    else if (LHS->get_type()->type_type() == TypeType::Pointer)
      set_to = LHS->gen_val();
    else
      set_to = LHS->gen_ptr();
    LLVMValueRef val = RHS->gen_val();
    LLVMBuildStore(curr_builder, val, set_to);
    return ptr ? set_to : val;
  }
  LLVMValueRef gen_val() {
    Type *lhs_t = LHS->get_type();
    PointerType *lhs_pt = dynamic_cast<PointerType *>(lhs_t);
    if (op == '=')
      return gen_assign(false);
    Type *rhs_t = RHS->get_type();
    NumType *lhs_nt = dynamic_cast<NumType *>(lhs_t);
    NumType *rhs_nt = dynamic_cast<NumType *>(rhs_t);
    PointerType *rhs_pt = dynamic_cast<PointerType *>(rhs_t);

    LLVMValueRef L = LHS->gen_val();
    LLVMValueRef R = RHS->gen_val();
    if (lhs_nt && rhs_nt)
      return gen_num_num_binop(op, L, R, lhs_nt, rhs_nt);
    else if (lhs_nt && rhs_pt)
      return gen_ptr_num_binop(op, R, L, rhs_pt, lhs_nt);
    else if (lhs_pt && rhs_nt)
      return gen_ptr_num_binop(op, L, R, lhs_pt, rhs_nt);
    error("Unknown ptr_ptr op");
    return nullptr;
  }
  LLVMValueRef gen_ptr() {
    if (op == '=')
      return gen_assign(true);
    else
      return ExprAST::gen_ptr();
  }
};
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  char op;
  ExprAST *operand;
  Type *type;

public:
  UnaryExprAST(char op, ExprAST *operand) : op(op), operand(operand) {
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
  LLVMValueRef gen_val() {
    auto zero =
        LLVMConstInt((new NumType(32, false, false))->llvm_type(), 0, false);
    switch (op) {
    case '!':
      // shortcut for != 1
      return LLVMBuildFCmp(curr_builder, LLVMRealONE, operand->gen_val(),
                           LLVMConstReal(float_64_type, 1.0), "nottmp");
    case '-':
      // shortcut for 0-n
      return LLVMBuildFSub(curr_builder, LLVMConstReal(float_64_type, 0.0),
                           operand->gen_val(), "negtmp");
    case '*': {
      PointerType *pt = dynamic_cast<PointerType *>(operand->get_type());
      if (!pt)
        error("* did not receive a pointer");
      return LLVMBuildLoad2(curr_builder, pt->get_points_to()->llvm_type(),
                            operand->gen_val(), "loadtmp");
    }
    case '&':
      if (operand->get_type()->type_type() == TypeType::Function)
        error("Can't get pointer to a function. When passing a function as an "
              "argument, you don't need to use '&func'!");
      return operand->gen_ptr();
    default:
      fprintf(stderr, "Error: invalid prefix unary operator '%c'", op);
      exit(1);
    }
  }
  LLVMValueRef gen_ptr() {
    if (op == '*') // fold ptr->val->ptr
      return operand->gen_val();
    else
      return ExprAST::gen_ptr();
  }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  FunctionType *func_t;
  ExprAST *called;
  ExprAST **args;
  unsigned int args_len;
  bool is_ptr;
  Type *type;

public:
  CallExprAST(ExprAST *called, ExprAST **args, unsigned int args_len)
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
    if (args_len != func_t->arguments_len) {

      fprintf(stderr,
              "Error: Incorrect # arguments passed. (Expected %d, got %d)",
              func_t->arguments_len, args_len);
      exit(1);
    }

    type = func_t->return_type;
  }

  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    LLVMValueRef func = called->gen_val();
    if (!func)
      error("Unknown function referenced");
    LLVMValueRef *arg_vs = alloc_arr<LLVMValueRef>(args_len);
    for (unsigned i = 0; i < args_len; i++)
      arg_vs[i] = args[i]->gen_val();
    return LLVMBuildCall2(curr_builder, func_t->llvm_type(), func, arg_vs,
                          args_len,
                          type->type_type() == TypeType::Void ? "" : "calltmp");
  }
};

/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  ExprAST *source;
  StructType *source_type;
  unsigned int index;
  Type *type;

public:
  PropAccessExprAST(char *key, unsigned int key_len, ExprAST *source)
      : source(source) {
    source_type = dynamic_cast<StructType *>(
        dynamic_cast<PointerType *>(source->get_type())->get_points_to());
    index = source_type->get_index(key, key_len);
    type = source_type->get_elem_type(index);
  }

  Type *get_type() { return type; }

  LLVMValueRef gen_val() {
    return LLVMBuildLoad2(curr_builder, type->llvm_type(), gen_ptr(),
                          "tmpload");
  }
  LLVMValueRef gen_ptr() {
    return LLVMBuildStructGEP2(curr_builder, source_type->llvm_type(),
                               source->gen_val(), index, "tmpgep");
  }
};

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {
  StructType *type;
  unsigned int *indexes;
  ExprAST **values;
  unsigned int key_count;

public:
  NewExprAST(char *name, unsigned int name_len, char **keys,
             unsigned int *key_lens, ExprAST **values, unsigned int key_count)
      : values(values), key_count(key_count) {
    type = dynamic_cast<StructType *>(
        curr_named_structs[std::string(name, name_len)]);
    indexes = alloc_arr<unsigned int>(key_count);
    for (unsigned int i = 0; i < key_count; i++)
      indexes[i] = type->get_index(keys[i], key_lens[i]);
  }
  NewExprAST(StructType *type, char **keys, unsigned int *key_lens,
             ExprAST **values, unsigned int key_count)
      : type(type), values(values), key_count(key_count) {
    indexes = alloc_arr<unsigned int>(key_count);
    for (unsigned int i = 0; i < key_count; i++)
      indexes[i] = type->get_index(keys[i], key_lens[i]);
  }

  Type *get_type() { return type; }

  LLVMValueRef gen_val() {
    LLVMValueRef ptr =
        LLVMBuildAlloca(curr_builder, type->llvm_type(), "newalloc");
    for (unsigned int i = 0; i < key_count; i++) {
      LLVMValueRef llvm_indexes[2] = {
          LLVMConstInt(LLVMInt32Type(), 0, false),
          LLVMConstInt(LLVMInt32Type(), indexes[i], false)};

      LLVMValueRef set_ptr = LLVMBuildStructGEP2(
          curr_builder, type->llvm_type(), ptr, indexes[i],
          "tmpgep"); // LLVMBuildGEP2(curr_builder, type->llvm_type(), ptr,
                     // llvm_indexes, key_count + 1, "tmpgep");
      LLVMBuildStore(curr_builder, values[i]->gen_val(), set_ptr);
    }
    return ptr;
  }
};

class BlockExprAST : public ExprAST {
  ExprAST **exprs;
  unsigned int exprs_len;
  Type *type;

public:
  BlockExprAST(ExprAST **exprs, unsigned int exprs_len)
      : exprs(exprs), exprs_len(exprs_len) {
    if (exprs_len == 0)
      error("block can't be empty.");
    type = exprs[exprs_len - 1]->get_type();
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    // generate code for all exprs and only return last expr
    for (unsigned int i = 0; i < exprs_len - 1; i++)
      exprs[i]->gen_val();
    return exprs[exprs_len - 1]->gen_val();
  }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  ExprAST *cond, *then, *elze;
  Type *type;

public:
  IfExprAST(ExprAST *cond, ExprAST *then,
            // elze because else cant be a variable name lol
            ExprAST *elze)
      : cond(cond), then(then), elze(elze) {
    Type *then_t = then->get_type();
    Type *else_t = elze->get_type();
    if (then_t->neq(else_t))
      error("if's then and else side don't have the same type");
    type = then_t;
  }

  Type *get_type() { return type; }

  LLVMValueRef gen_val() {
    LLVMValueRef cond_v = cond->gen_val();
    if (NumType *n = dynamic_cast<NumType *>(cond->get_type()))
      if (n->is_floating)
        cond_v = LLVMBuildFCmp(curr_builder, LLVMRealONE, cond_v,
                               LLVMConstReal(float_64_type, 0.0), "ifcond");
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
    LLVMBasicBlockRef then_bb =
        LLVMAppendBasicBlockInContext(curr_ctx, func, "ifthen");
    LLVMBasicBlockRef else_bb =
        LLVMCreateBasicBlockInContext(curr_ctx, "ifelse");
    LLVMBasicBlockRef merge_bb =
        LLVMCreateBasicBlockInContext(curr_ctx, "ifmerge");
    // if
    LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
    // then
    LLVMPositionBuilderAtEnd(curr_builder, then_bb);
    LLVMValueRef then_v = then->gen_val();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'then' can change the current block, update then_bb for the
    // PHI.
    then_bb = LLVMGetInsertBlock(curr_builder);
    // else
    LLVMAppendExistingBasicBlock(func, else_bb);
    LLVMPositionBuilderAtEnd(curr_builder, else_bb);
    LLVMValueRef else_v = elze->gen_val();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'else' can change the current block, update else_bb for the
    // PHI.
    else_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    LLVMValueRef phi =
        LLVMBuildPhi(curr_builder, get_type()->llvm_type(), "ifphi");
    // todo merge idk
    LLVMAddIncoming(phi, &then_v, &then_bb, 1);
    LLVMAddIncoming(phi, &else_v, &else_bb, 1);
    return phi;
  }
};

/// WhileExprAST - Expression class for while loops.
class WhileExprAST : public ExprAST {
  ExprAST *cond, *then, *elze;
  Type *type;

public:
  WhileExprAST(ExprAST *cond, ExprAST *then, ExprAST *elze)
      : cond(cond), then(then), elze(elze) {
    Type *then_t = then->get_type();
    Type *else_t = elze->get_type();
    if (then_t->neq(else_t)) {
      fprintf(stderr,
              "Error: while's then and else side don't have the same type: ");
      then_t->log_diff(else_t);
      exit(1);
    }
    type = then_t;
  }

  Type *get_type() { return type; }

  LLVMValueRef gen_val() {
    LLVMValueRef cond_v = cond->gen_val();
    if (NumType *n = dynamic_cast<NumType *>(cond->get_type()))
      if (n->is_floating)
        cond_v = LLVMBuildFCmp(curr_builder, LLVMRealONE, cond_v,
                               LLVMConstReal(float_64_type, 0.0), "whilecond");
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
    LLVMBasicBlockRef then_bb =
        LLVMAppendBasicBlockInContext(curr_ctx, func, "whilethen");
    LLVMBasicBlockRef else_bb =
        LLVMCreateBasicBlockInContext(curr_ctx, "whileelse");
    LLVMBasicBlockRef merge_bb =
        LLVMCreateBasicBlockInContext(curr_ctx, "endwhile");
    // while
    LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
    // then
    LLVMPositionBuilderAtEnd(curr_builder, then_bb);
    LLVMValueRef then_v = then->gen_val();
    LLVMValueRef cond_v2 = cond->gen_val();
    if (NumType *n = dynamic_cast<NumType *>(cond->get_type()))
      if (n->is_floating)
        cond_v2 = LLVMBuildFCmp(curr_builder, LLVMRealONE, cond_v,
                                LLVMConstReal(float_64_type, 0.0), "whilecond");
    LLVMBuildCondBr(curr_builder, cond_v2, then_bb, merge_bb);
    // Codegen of 'then' can change the current block, update then_bb for the
    // PHI.
    then_bb = LLVMGetInsertBlock(curr_builder);
    // else
    LLVMAppendExistingBasicBlock(func, else_bb);
    LLVMPositionBuilderAtEnd(curr_builder, else_bb);
    LLVMValueRef else_v = elze->gen_val();
    LLVMBuildBr(curr_builder, merge_bb);
    // Codegen of 'else' can change the current block, update else_bb for the
    // PHI.
    else_bb = LLVMGetInsertBlock(curr_builder);
    // merge
    LLVMAppendExistingBasicBlock(func, merge_bb);
    LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
    LLVMValueRef phi =
        LLVMBuildPhi(curr_builder, get_type()->llvm_type(), "whilephi");
    // todo merge idk
    LLVMAddIncoming(phi, &then_v, &then_bb, 1);
    LLVMAddIncoming(phi, &else_v, &else_bb, 1);
    return phi;
  }
};
/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
public:
  char **arg_names;
  unsigned int *arg_name_lengths;
  Type **arg_types;
  Type *return_type;
  FunctionType *type;
  unsigned int arg_count;
  char *name;
  unsigned int name_len;
  PrototypeAST(char *name, unsigned int name_len, char **arg_names,
               unsigned int *arg_name_lengths, Type **arg_types,
               unsigned int arg_count, Type *return_type)
      : name(name), name_len(name_len), arg_names(arg_names),
        arg_name_lengths(arg_name_lengths), arg_types(arg_types),
        arg_count(arg_count), return_type(return_type) {
    for (unsigned i = 0; i != arg_count; ++i)
      curr_named_var_types[std::string(arg_names[i], arg_name_lengths[i])] =
          arg_types[i];
    curr_named_var_types[std::string(name, name_len)] = type =
        new FunctionType(return_type, arg_types, arg_count);
  }
  FunctionType *get_type() { return type; }
  LLVMValueRef codegen() {
    // Make the function type:  double(double,double) etc.
    LLVMTypeRef *llvm_arg_types = alloc_arr<LLVMTypeRef>(arg_count);
    for (unsigned i = 0; i != arg_count; ++i) {
      llvm_arg_types[i] = arg_types[i]->llvm_type();
    }

    LLVMTypeRef function_type = LLVMFunctionType(
        return_type->llvm_type(), llvm_arg_types, arg_count, false);

    LLVMValueRef func = LLVMAddFunction(curr_module, name, function_type);
    curr_named_variables[std::string(name, name_len)] =
        new ConstVariable(func, nullptr);
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
/// ExternExprAST - Expression class for defining an extern.
class ExternExprAST : public TopLevelAST {
  LetExprAST *let = nullptr;
  PrototypeAST *prot = nullptr;
  Type *type;

public:
  ExternExprAST(LetExprAST *let) : let(let) {
    type = let->get_type();
    register_extern();
  }
  ExternExprAST(PrototypeAST *prot) : prot(prot) {
    type = prot->get_type();
    register_extern();
  }
  void register_extern() {
    if (FunctionType *fun_t = dynamic_cast<FunctionType *>(type))
      curr_named_var_types[std::string(prot->name, prot->name_len)] = fun_t;
    else
      curr_named_var_types[std::string(let->id, let->id_len)] = type;
  }
  void gen_toplevel() {
    register_extern();
    if (let)
      let->gen_declare();
    else
      prot->codegen();
  }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public TopLevelAST {
  PrototypeAST *proto;
  ExprAST *body;

public:
  FunctionAST(PrototypeAST *proto, ExprAST *body) {
    if (proto->return_type == nullptr)
      proto->return_type = body->get_type();
    this->proto = proto;
    this->body = body;
  }

  void gen_toplevel() {
    // First, check for an existing function from a previous 'extern'
    // declaration.
    LLVMValueRef func = LLVMGetNamedFunction(curr_module, proto->name);

    if (!func)
      func = proto->codegen();

    if (!func) {
      error("funcless behavior");
      return;
    }

    if (LLVMCountBasicBlocks(func) != 0)
      error("Function cannot be redefined.");

    auto block = LLVMAppendBasicBlockInContext(curr_ctx, func, "");
    LLVMPositionBuilderAtEnd(curr_builder, block);

    unsigned int args_len = LLVMCountParams(func);
    LLVMValueRef *params = alloc_arr<LLVMValueRef>(args_len);
    LLVMGetParams(func, params);
    size_t unused = 0;
    for (unsigned i = 0; i != args_len; ++i)
      curr_named_variables[LLVMGetValueName2(params[i], &unused)] =
          new ConstVariable(params[i], nullptr);
    LLVMValueRef ret_val = body->gen_val();
    // Finish off the function.
    LLVMBuildRet(curr_builder, ret_val);
    // doesnt exist in c api (i think)
    // // Validate the generated code, checking for consistency.
    // // verifyFunction(*TheFunction);
  }
};

class StructAST : public TopLevelAST {
  char *name;
  unsigned int name_len;
  char **names;
  unsigned int *name_lengths;
  Type **types;
  unsigned int count;

public:
  StructAST(char *name, unsigned int name_len, char **names,
            unsigned int *name_lengths, Type **types, unsigned int count)
      : name(name), name_len(name_len), names(names),
        name_lengths(name_lengths), types(types), count(count) {}
  void gen_toplevel() {
    curr_named_structs[std::string(name, name_len)] =
        new StructType(name, name_len, names, name_lengths, types, count);
  }
};