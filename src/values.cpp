#pragma once
#include "types.cpp"
#include "utils.cpp"
/// Variable - Base class for variable info.
class Value {
public:
  virtual Type *get_type() = 0;
  virtual LLVMValueRef gen_load() = 0;
  virtual LLVMValueRef gen_ptr() = 0;
};
/// ConstValue - For when everything always returns the same thing.
class ConstValue : public Value {
public:
  LLVMValueRef const_load;
  LLVMValueRef const_ptr;
  Type *type;
  ConstValue(Type *type, LLVMValueRef const_load, LLVMValueRef const_ptr)
      : type(type), const_load(const_load), const_ptr(const_ptr) {}
  Type *get_type() { return type; };
  LLVMValueRef gen_load() {
    if (!const_load)
      error("No const load set");
    return const_load;
  };
  LLVMValueRef gen_ptr() {
    if (!const_ptr)
      error("No const ptr set");
    return const_ptr;
  };
};
/// BasicLoadValue - generates a load op.
class BasicLoadValue : public Value {
public:
  LLVMValueRef variable;
  Type *type;
  BasicLoadValue(LLVMValueRef variable, Type *type)
      : variable(variable), type(type) {}
  Type *get_type() { return type; }
  LLVMValueRef gen_load() {
    return LLVMBuildLoad2(curr_builder, type->llvm_type(), variable, "");
  };
  LLVMValueRef gen_ptr() { return variable; };
};
class PHIValue : public Value {
public:
  LLVMBasicBlockRef a_bb;
  Value *a_v;
  LLVMBasicBlockRef b_bb;
  Value *b_v;
  Type *type;
  PHIValue(LLVMBasicBlockRef a_bb, Value *a_v, LLVMBasicBlockRef b_bb,
           Value *b_v)
      : a_bb(a_bb), a_v(a_v), b_bb(b_bb), b_v(b_v) {
    type = a_v->get_type(); // TODO
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_load() {
    LLVMValueRef phi = LLVMBuildPhi(curr_builder, get_type()->llvm_type(), "");
    LLVMValueRef incoming_v[2] = {a_v->gen_load(), b_v->gen_load()};
    LLVMBasicBlockRef incoming_bb[2] = {a_bb, b_bb};
    LLVMAddIncoming(phi, incoming_v, incoming_bb, 2);
    return phi;
  }
  LLVMValueRef gen_ptr() {
    LLVMValueRef phi = LLVMBuildPhi(curr_builder, get_type()->llvm_type(), "");
    LLVMValueRef incoming_v[2] = {a_v->gen_ptr(), b_v->gen_ptr()};
    LLVMBasicBlockRef incoming_bb[2] = {a_bb, b_bb};
    LLVMAddIncoming(phi, incoming_v, incoming_bb, 2);
    return phi;
  }
};

LLVMValueRef gen_num_cast(LLVMValueRef value, NumType *a, Type *b) {
  if (NumType *num = dynamic_cast<NumType *>(b)) {
    if (!num->is_floating && a->is_floating)
      return LLVMBuildCast(curr_builder, a->is_signed ? LLVMFPToSI : LLVMFPToUI,
                           value, b->llvm_type(), "");
    if (num->is_floating && !a->is_floating)
      return LLVMBuildCast(curr_builder, a->is_signed ? LLVMSIToFP : LLVMUIToFP,
                           value, b->llvm_type(), "");
    if (a->is_floating)
      return LLVMBuildFPCast(curr_builder, value, num->llvm_type(), "");
    return LLVMBuildIntCast2(curr_builder, value, num->llvm_type(),
                             a->is_signed, "");
  }
  error("Numbers can't be casted to non-numbers yet");
}

LLVMValueRef gen_ptr_cast(LLVMValueRef value, PointerType *a, Type *b) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(b))
    return LLVMBuildPointerCast(curr_builder, value, ptr->llvm_type(), "");
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
        LLVMBuildGEP2(curr_builder, a->llvm_type(), value, zeros, 2, "");
    return cast;
  }
  error("Arrays can't be casted to non-pointers yet");
}

LLVMValueRef cast(LLVMValueRef source, Type *src, Type *to) {
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
  LLVMValueRef gen_load() {
    return cast(source->gen_load(), source->get_type(), to);
  }
  LLVMValueRef gen_ptr() {
    return cast(source->gen_ptr(), new PointerType(source->get_type()),
                new PointerType(source->get_type()));
  }
};