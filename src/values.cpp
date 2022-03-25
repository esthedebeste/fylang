#pragma once
#include "types.cpp"
#include "utils.cpp"
class CastValue;
/// Variable - Base class for variable info.
class Value {
public:
  virtual Type *get_type() = 0;
  virtual LLVMValueRef gen_val() = 0;
  virtual LLVMValueRef gen_ptr() = 0;
  virtual bool has_ptr() { return true; }
  CastValue *cast_to(Type *type);
};
/// ConstValue - Constant value with no pointer.
class ConstValue : public Value {
public:
  LLVMValueRef val;
  Type *type;
  ConstValue(Type *type, LLVMValueRef val) : type(type), val(val) {}
  Type *get_type() { return type; };
  LLVMValueRef gen_val() { return val; };
  LLVMValueRef gen_ptr() { error("Const values can't be pointered"); };
  bool has_ptr() { return false; }
};
/// FuncValue - For functions
class FuncValue : public Value {
public:
  LLVMValueRef func;
  Type *type;
  FuncValue(Type *type, LLVMValueRef func) : type(type), func(func) {}
  Type *get_type() { return type; };
  LLVMValueRef gen_val() { return func; };
  LLVMValueRef gen_ptr() { return func; };
  bool has_ptr() { return true; }
};
/// BasicLoadValue - generates a load op.
class BasicLoadValue : public Value {
public:
  LLVMValueRef variable;
  Type *type;
  BasicLoadValue(LLVMValueRef variable, Type *type)
      : variable(variable), type(type) {}
  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    return LLVMBuildLoad2(curr_builder, type->llvm_type(), variable, UN);
  };
  LLVMValueRef gen_ptr() { return variable; };
  bool has_ptr() { return true; }
};
class PHIValue : public Value {
public:
  LLVMBasicBlockRef a_bb;
  Value *a_v;
  LLVMBasicBlockRef b_bb;
  Value *b_v;
  Type *type;
  LLVMValueRef load = nullptr;
  LLVMValueRef ptr = nullptr;
  PHIValue(LLVMBasicBlockRef a_bb, Value *a_v, LLVMBasicBlockRef b_bb,
           Value *b_v)
      : a_bb(a_bb), a_v(a_v), b_bb(b_bb), b_v(b_v) {
    type = a_v->get_type(); // TODO
    gen_val();
    if (a_v->has_ptr() && b_v->has_ptr())
      gen_ptr();
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_val() {
    if (load)
      return load;
    LLVMValueRef incoming_v[2] = {a_v->gen_val(), b_v->gen_val()};
    LLVMBasicBlockRef incoming_bb[2] = {a_bb, b_bb};
    LLVMValueRef phi = LLVMBuildPhi(curr_builder, get_type()->llvm_type(), UN);
    LLVMAddIncoming(phi, incoming_v, incoming_bb, 2);
    return load = phi;
  }
  LLVMValueRef gen_ptr() {
    if (ptr)
      return ptr;
    else
      error("conditional does not have a pointer-ish value on both sides");
  }
  bool has_ptr() { return ptr != nullptr; }
};

LLVMValueRef gen_num_cast(LLVMValueRef value, NumType *a, Type *b) {
  if (NumType *num = dynamic_cast<NumType *>(b)) {
    if (num->bits == 1) {
      LLVMValueRef zero = LLVMConstNull(a->llvm_type());
      if (a->is_floating)
        return LLVMBuildFCmp(curr_builder, LLVMRealPredicate::LLVMRealUNE,
                             value, zero, UN);
      else
        return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntNE, value,
                             zero, UN);
    }
    if (!num->is_floating && a->is_floating)
      return LLVMBuildCast(curr_builder, a->is_signed ? LLVMFPToSI : LLVMFPToUI,
                           value, b->llvm_type(), UN);
    if (num->is_floating && !a->is_floating)
      return LLVMBuildCast(curr_builder, a->is_signed ? LLVMSIToFP : LLVMUIToFP,
                           value, b->llvm_type(), UN);
    if (a->is_floating)
      return LLVMBuildFPCast(curr_builder, value, num->llvm_type(), UN);
    return LLVMBuildIntCast2(curr_builder, value, num->llvm_type(),
                             num->is_signed, UN);
  } else if (b->type_type() == TypeType::Pointer) {
    if (a->byte_size == LLVMPointerSize(target_data))
      return LLVMBuildBitCast(curr_builder, value, b->llvm_type(), UN);
    else
      error("Can only cast a pointersize number to a pointer");
  }
  error("Numbers can't be casted to non-numbers yet");
}

LLVMValueRef gen_ptr_cast(LLVMValueRef value, PointerType *a, Type *b) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(b))
    return LLVMBuildPointerCast(curr_builder, value, ptr->llvm_type(), UN);
  else if (NumType *num = dynamic_cast<NumType *>(b)) {
    if (num->byte_size == LLVMPointerSize(target_data))
      return LLVMBuildBitCast(curr_builder, value, b->llvm_type(), UN);
    else
      error("Can't cast a pointer to a non-pointersize number");
  }
  error("Pointers can't be casted to non-pointers yet");
}

LLVMValueRef gen_arr_cast(LLVMValueRef value, TupleType *a, Type *b) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(b)) {
    if (ptr->get_points_to()->neq(a->get_elem_type())) {
      ptr->get_points_to()->log_diff(a->get_elem_type());
      error("Tuple can't be casted to pointer with different type");
    }
    LLVMValueRef zeros[2] = {
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false),
        LLVMConstInt((new NumType(64, false, false))->llvm_type(), 0, false)};
    // cast [ ... x T ]* to T*
    LLVMValueRef cast =
        LLVMBuildGEP2(curr_builder, a->llvm_type(), value, zeros, 2, UN);
    return cast;
  }
  error("Arrays can't be casted to non-pointers yet");
}

LLVMValueRef cast(LLVMValueRef source, Type *src, Type *to) {
  if (src->eq(to))
    return source;
  if (NumType *num = dynamic_cast<NumType *>(src))
    return gen_num_cast(source, num, to);
  if (PointerType *ptr = dynamic_cast<PointerType *>(src))
    return gen_ptr_cast(source, ptr, to);
  if (TupleType *tup = dynamic_cast<TupleType *>(src))
    return gen_arr_cast(source, tup, to);
  error("Invalid cast");
}

class CastValue : public Value {
public:
  Value *source;
  Type *to;
  CastValue(Value *source, Type *to) : source(source), to(to) {}
  Type *get_type() { return to; }
  LLVMValueRef gen_val() {
    return cast(source->gen_val(), source->get_type(), to);
  }
  LLVMValueRef gen_ptr() {
    return cast(source->gen_ptr(), new PointerType(source->get_type()),
                new PointerType(source->get_type()));
  }
  bool has_ptr() { return source->has_ptr(); }
};
CastValue *Value::cast_to(Type *to) { return new CastValue(this, to); }

class NamedValue : public Value {
public:
  Value *val;
  char *name;
  size_t name_len;
  NamedValue(Value *val, char *name, size_t name_len)
      : val(val), name(name), name_len(name_len) {}
  Type *get_type() { return val->get_type(); }
  LLVMValueRef gen_val() {
    LLVMValueRef value = val->gen_val();
    LLVMSetValueName2(value, name, name_len);
    return value;
  }
  LLVMValueRef gen_ptr() {
    LLVMValueRef value = val->gen_ptr();
    LLVMSetValueName2(value, name, name_len);
    return value;
  }
  bool has_ptr() { return val->has_ptr(); }
};