#pragma once
#include "../lexer.h"
#include "../types.h"
#include "../utils.h"
#include "../values.h"
#include "functions.h"
#include "types.h"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST();
  virtual Type *get_type() = 0;
  virtual Value *gen_value() = 0;
  virtual bool is_constant();
};

struct LoopState {
  LLVMBasicBlockRef break_block;
  LLVMBasicBlockRef continue_block;
};
extern std::vector<LoopState> loop_stack;

Value *build_malloc(Type *type);

/// SizeofExprAST - Expression class to get the byte size of a type
class SizeofExprAST : public ExprAST {
public:
  TypeAST *type;
  SizeofExprAST(TypeAST *type);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  union {
    long double floating;
    unsigned long long integer;
  } value;

public:
  NumType type;
  NumberExprAST(std::string val, char type_char, bool has_dot,
                unsigned int base);
  NumberExprAST(unsigned long long val, char type_char);
  NumberExprAST(unsigned long long val, NumType type);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};
/// BoolExprAST - Expression class for boolean literals (true or false).
class BoolExprAST : public ExprAST {
  bool value;

public:
  BoolExprAST(bool value);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

class CastExprAST : public ExprAST {
public:
  ExprAST *value;
  TypeAST *to;
  CastExprAST(ExprAST *value, TypeAST *to);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

#include "scope.h"
/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {

public:
  Identifier name;
  VariableExprAST(Identifier name);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

extern std::vector<std::pair<LLVMValueRef, ExprAST *>> inits;
void add_store_before_main(LLVMValueRef ptr, ExprAST *val);
#include "../ucr.h"
void add_stores_before_main(LLVMValueRef main_func);

/// LetExprAST - Expression class for creating a variable, like "let a = 3".
class LetExprAST : public ExprAST {

public:
  std::string id;
  bool untyped;
  TypeAST *type;
  ExprAST *value;
  bool constant;
  LetExprAST(std::string id, TypeAST *type, ExprAST *value, bool constant);
  LLVMValueRef gen_toplevel();
  Value *gen_value();
  Type *get_type();
  LLVMValueRef gen_declare();
};

/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public NumberExprAST {
public:
  CharExprAST(char data);
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

template <typename CharT> class PtrStringExprAST : public ExprAST {
  inline static NumType char_type{NumType(sizeof(CharT) * 8, false, false)};
  ArrayType t_type;
  PointerType p_type;

public:
  std::basic_string<CharT> str;
  bool null_terminated;
  PtrStringExprAST(std::basic_string<CharT> str, bool null_terminated)
      : str(str), t_type(&char_type, str.size() + null_terminated),
        p_type(&this->t_type), null_terminated(null_terminated) {}
  Type *get_type() { return &p_type; }
  Value *gen_value() {
    auto len = str.size();
    LLVMValueRef *vals = new LLVMValueRef[len + null_terminated];
    for (size_t i = 0; i < len; i++)
      vals[i] = LLVMConstInt(char_type.llvm_type(), str[i], false);
    if (null_terminated)
      vals[len] = LLVMConstNull(char_type.llvm_type());
    auto array =
        LLVMConstArray(char_type.llvm_type(), vals, len + null_terminated);
    LLVMValueRef ptr = LLVMAddGlobal(curr_module, t_type.llvm_type(),
                                     null_terminated ? ".c_str" : ".str");
    LLVMSetInitializer(ptr, array);
    LLVMSetGlobalConstant(ptr, true);
    LLVMSetLinkage(ptr, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(ptr, LLVMGlobalUnnamedAddr);
    LLVMSetAlignment(ptr, sizeof(CharT));
    LLVMValueRef cast =
        LLVMBuildBitCast(curr_builder, ptr, p_type.llvm_type(), UN);
    return new ConstValue(&p_type, cast);
  }
  bool is_constant() { return true; }
};

LLVMValueRef gen_num_num_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               NumType *lhs_nt, NumType *rhs_nt);
LLVMValueRef gen_ptr_num_binop(int op, LLVMValueRef ptr, LLVMValueRef num,
                               PointerType *ptr_t, NumType *num_t);
LLVMValueRef gen_ptr_ptr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               PointerType *lhs_ptr, PointerType *rhs_pt);
LLVMValueRef gen_arr_arr_binop(int op, LLVMValueRef L, LLVMValueRef R,
                               ArrayType *lhs_at, ArrayType *rhs_at);
Type *get_binop_type(int op, Type *lhs_t, Type *rhs_t);
Value *gen_binop(int op, LLVMValueRef L, LLVMValueRef R, Type *lhs_t,
                 Type *rhs_t);

class AssignExprAST : public ExprAST {
  ExprAST *LHS, *RHS;

public:
  AssignExprAST(ExprAST *LHS, ExprAST *RHS);
  Type *get_type();
  Value *gen_value();
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST *LHS, *RHS;

public:
  BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS);

  Type *get_type();

  Value *gen_value();
  bool is_constant();
};
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  int op;
  ExprAST *operand;

public:
  UnaryExprAST(int op, ExprAST *operand);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

/// ValueCallExprAST - For calling a value (often a function pointer)
class ValueCallExprAST : public ExprAST {
  ExprAST *called;
  std::vector<ExprAST *> args;
  bool is_ptr;
  FunctionType *get_func_type();

public:
  ValueCallExprAST(ExprAST *called, std::vector<ExprAST *> args);

  Type *get_type();
  Value *gen_value();
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
             std::vector<std::pair<std::string, ExprAST *>> args);
  ASMExprAST(std::string asm_str,
             std::vector<std::pair<std::string, ExprAST *>> args);

  Type *get_type();
  Value *gen_value();
};

/// GlobalASMExprAST - Module-level inline assembly
class GlobalASMExprAST {
  std::string asm_str;

public:
  GlobalASMExprAST(std::string asm_str);
  void gen_toplevel();
};

class NameCallExprAST : public ExprAST {
public:
  Identifier name;
  std::vector<ExprAST *> args;
  NameCallExprAST(Identifier name, std::vector<ExprAST *> args);
  Type *get_type();
  Value *gen_value();
};

/// IndexExprAST - Expression class for accessing indexes (a[0]).
class IndexExprAST : public ExprAST {
  ExprAST *value;
  ExprAST *index;

public:
  IndexExprAST(ExprAST *value, ExprAST *index);
  Type *get_type();
  Value *gen_value();
};

/// NumAccessExprAST - Expression class for accessing indexes on Tuples (a.0).
class NumAccessExprAST : public ExprAST {
  bool is_ptr;
  TupleType *source_type;

public:
  unsigned int index;
  ExprAST *source;
  NumAccessExprAST(unsigned int index, ExprAST *source);
  Type *get_type();
  Value *gen_value();
};

/// PropAccessExprAST - Expression class for accessing properties (a.size).
class PropAccessExprAST : public ExprAST {
  bool is_ptr;
  StructType *source_type;
  unsigned int index;

public:
  std::string key;
  ExprAST *source;
  PropAccessExprAST(std::string key, ExprAST *source);

  Type *get_type();
  Value *gen_value();
};

struct ExtAndPtr {
  MethodAST *extension;
  bool is_ptr;
};
/// MethodCallExprAST - Expression class for calling methods (a.len()).
class MethodCallExprAST : public ExprAST {
  ExtAndPtr get_extension();

public:
  std::string name;
  ExprAST *source;
  std::vector<ExprAST *> args;
  MethodCallExprAST(std::string name, ExprAST *source,
                    std::vector<ExprAST *> args);

  Type *get_type();
  Value *gen_value();
};

/// NewExprAST - Expression class for creating an instance of a struct (new
/// String { pointer = "hi", length = 2 } ).
class NewExprAST : public ExprAST {

public:
  TypeAST *s_type;
  std::vector<std::pair<std::string, ExprAST *>> fields;
  bool is_new;
  NewExprAST(TypeAST *s_type,
             std::vector<std::pair<std::string, ExprAST *>> fields,
             bool is_new);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

class TupleExprAST : public ExprAST {
  std::vector<ExprAST *> values;
  TupleType *t_type;

public:
  bool is_new = false;
  TupleExprAST(std::vector<ExprAST *> values);
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

class BlockExprAST : public ExprAST {

public:
  std::vector<ExprAST *> exprs;
  BlockExprAST(std::vector<ExprAST *> exprs);
  Type *get_type();
  Value *gen_value();
};

ConstValue *null_value(Type *type = &null_type);
/// NullExprAST - null
class NullExprAST : public ExprAST {
public:
  Type *type;
  NullExprAST(Type *type);
  NullExprAST();
  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

class TypeAssertExprAST : public ExprAST {
public:
  TypeAST *a;
  TypeAST *b;
  TypeAssertExprAST(TypeAST *a, TypeAST *b);
  Type *get_type();
  Value *gen_value();
};

class TypeDumpExprAST : public ExprAST {
public:
  TypeAST *type;
  TypeDumpExprAST(TypeAST *type);
  Type *get_type();
  Value *gen_value();
};

/// TypeIfExprAST - Expression class for ifs based on type.
class TypeIfExprAST : public ExprAST {
  ExprAST *pick();

public:
  ExprAST *then, *elze;
  TypeAST *a, *b;
  TypeIfExprAST(TypeAST *a, TypeAST *b, ExprAST *then,
                // elze because else cant be a variable name lol
                ExprAST *elze);

  Type *get_type();
  Value *gen_value();
  bool is_constant();
};

/// OrExprAST - Expression class for a short-circuiting or
class OrExprAST : public ExprAST {
public:
  ExprAST *left, *right;
  OrExprAST(ExprAST *left, ExprAST *right);
  Type *get_type();
  Value *gen_value();
};

/// AndExprAST - Expression class for a short-circuiting and
class AndExprAST : public OrExprAST {
public:
  using OrExprAST::OrExprAST;
  Value *gen_value();
};

/// ContinueExprAST - Expression class for skipping to the next iteration
class ContinueExprAST : public ExprAST {
public:
  ContinueExprAST();
  Type *get_type();
  Value *gen_value();
};
/// BreakExprAST - Expression class for breaking out of a loop
class BreakExprAST : public ExprAST {
public:
  BreakExprAST();
  Type *get_type();
  Value *gen_value();
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
  ExprAST *cond, *then, *elze;
  bool null_else;
  Type *type;
  void init();
  IfExprAST(ExprAST *cond, ExprAST *then,
            // elze because else cant be a variable name lol
            ExprAST *elze);

  Type *get_type();

  Value *gen_value();
};

/// WhileExprAST - Expression class for while loops.
class WhileExprAST : public ExprAST {
  ExprAST *cond, *body, *elze;

public:
  WhileExprAST(ExprAST *cond, ExprAST *body, ExprAST *elze);
  Type *get_type();
  Value *gen_value();
};

class ForExprAST : public ExprAST {
  ExprAST *init, *cond, *body, *post, *elze;

public:
  ForExprAST(ExprAST *init, ExprAST *cond, ExprAST *body, ExprAST *post,
             ExprAST *elze);

  Type *get_type();
  Value *gen_value();
};

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

/// DeclareExprAST - Expression class for defining a declare.
class DeclareExprAST {
  LetExprAST *let = nullptr;
  FunctionAST *func = nullptr;

public:
  DeclareExprAST(LetExprAST *let);
  DeclareExprAST(FunctionAST *func);
  LLVMValueRef gen_toplevel();
};