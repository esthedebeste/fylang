#pragma once
#include "types.h"
#include "utils.h"

class CastValue;
/// Value - Base class for value info.
class Value {
public:
  virtual Type *get_type() = 0;
  virtual LLVMValueRef gen_val() = 0;
  virtual LLVMValueRef gen_ptr() = 0;
  virtual bool has_ptr() = 0;
  CastValue *cast_to(Type *type);
};
/// ConstValue - Constant value with no pointer.
class ConstValue : public Value {
public:
  LLVMValueRef val;
  Type *type;
  ConstValue(Type *type, LLVMValueRef val);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};
/// ConstValueWithPtr - Constant value with a pointer to it's data.
class ConstValueWithPtr : public Value {
public:
  LLVMValueRef val;
  LLVMValueRef ptr;
  Type *type;
  ConstValueWithPtr(Type *type, LLVMValueRef ptr, LLVMValueRef val);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};
/// IntValue - Integer value.
class IntValue : public Value {
public:
  uint64_t val;
  NumType type;
  IntValue(NumType type, uint64_t val);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};
/// FuncValue - For functions
class FuncValue : public Value {
public:
  LLVMValueRef func;
  Type *type;
  FuncValue(Type *type, LLVMValueRef func);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};
/// BasicLoadValue - generates a load op.
class BasicLoadValue : public Value {
public:
  LLVMValueRef variable;
  Type *type;
  BasicLoadValue(Type *type, LLVMValueRef variable);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};

ConstValue *gen_phi(LLVMBasicBlockRef a_bb, Value *a_v, LLVMBasicBlockRef b_bb,
                    Value *b_v);

LLVMValueRef gen_num_cast(LLVMValueRef value, NumType *a, Type *b);
LLVMValueRef gen_ptr_cast(LLVMValueRef value, PointerType *a, Type *b);
LLVMValueRef gen_tuple_cast(Value *value, TupleType *a, Type *b);
LLVMValueRef cast(Value *source, Type *to);

class CastValue : public Value {
public:
  Value *source;
  Type *to;
  CastValue(Value *source, Type *to);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};

class NamedValue : public Value {
public:
  Value *val;
  std::string name;
  NamedValue(Value *val, std::string name);
  Type *get_type();
  LLVMValueRef gen_val();
  LLVMValueRef gen_ptr();
  bool has_ptr();
};
