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
  LLVMValueRef load;
  LLVMValueRef ptr = nullptr;
  PHIValue(LLVMBasicBlockRef a_bb, Value *a_v, LLVMBasicBlockRef b_bb,
           Value *b_v)
      : a_bb(a_bb), a_v(a_v), b_bb(b_bb), b_v(b_v) {
    if (a_v->get_type()->neq(b_v->get_type()))
      error("conditional's values must have the same type");
    type = a_v->get_type();
    bool mk_ptr = a_v->has_ptr() && b_v->has_ptr();
    LLVMBasicBlockRef curr = LLVMGetInsertBlock(curr_builder);
    LLVMPositionBuilderBefore(
        curr_builder, LLVMGetLastInstruction(a_bb)); /* before the terminator */
    LLVMValueRef a_val = a_v->gen_val();
    LLVMValueRef a_ptr = mk_ptr ? a_v->gen_ptr() : nullptr;
    LLVMPositionBuilderBefore(
        curr_builder, LLVMGetLastInstruction(b_bb)); /* before the terminator */
    LLVMValueRef b_val = b_v->gen_val();
    LLVMValueRef b_ptr = mk_ptr ? b_v->gen_ptr() : nullptr;
    LLVMValueRef incoming_v[2] = {a_val, b_val};
    LLVMValueRef incoming_p[2] = {a_ptr, b_ptr};
    LLVMBasicBlockRef incoming_bb[2] = {a_bb, b_bb};
    LLVMPositionBuilderAtEnd(curr_builder, curr);
    load = LLVMBuildPhi(curr_builder, get_type()->llvm_type(), UN);
    LLVMAddIncoming(load, incoming_v, incoming_bb, 2);
    if (mk_ptr) {
      ptr = LLVMBuildPhi(curr_builder, get_type()->ptr()->llvm_type(), UN);
      LLVMAddIncoming(ptr, incoming_p, incoming_bb, 2);
    }
  }
  Type *get_type() { return type; }
  LLVMValueRef gen_val() { return load; }
  LLVMValueRef gen_ptr() { return ptr; }
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
                             a->is_signed, UN);
  } else if (b->type_type() == TypeType::Pointer) {
    return LLVMBuildIntToPtr(curr_builder, value, b->llvm_type(), UN);
  }
  error("Numbers can't be casted to non-numbers yet");
}

LLVMValueRef gen_ptr_cast(LLVMValueRef value, PointerType *a, Type *b) {
  if (b->type_type() == TypeType::Pointer)
    return LLVMBuildPointerCast(curr_builder, value, b->llvm_type(), UN);
  else if (b->type_type() == TypeType::Number)
    return LLVMBuildPtrToInt(curr_builder, value, b->llvm_type(), UN);
  error("Pointers can't be casted to non-pointers yet");
}

LLVMValueRef gen_arr_cast(Value *value, ArrayType *a, Type *b) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(b)) {
    if (ptr->get_points_to()->neq(a->get_elem_type()))
      error("Array can't be casted to pointer with different type, "
            << a->get_elem_type()->stringify() << "[" << a->count
            << "] can't be casted to *" << ptr->get_points_to()->stringify()
            << ".");
    LLVMValueRef zeros[2] = {
        LLVMConstInt(NumType(false).llvm_type(), 0, false),
        LLVMConstInt(NumType(false).llvm_type(), 0, false)};
    // cast [ ... x T ]* to T*
    LLVMValueRef cast = LLVMBuildGEP2(curr_builder, a->llvm_type(),
                                      value->gen_ptr(), zeros, 2, UN);
    return cast;
  }
  error("Arrays can't be casted to non-pointers yet");
}

LLVMValueRef gen_str_cast(Value *string_value, Type *to) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(to)) {
    if (NumType(8, false, false).neq(ptr->get_points_to()))
      error("String can't be casted to " << ptr->stringify() << ".");
    LLVMValueRef str = string_value->gen_val();
    LLVMValueRef chars = LLVMBuildExtractValue(curr_builder, str, 0, UN);
    return chars;
  }
  error("Strings can only be casted to *char.");
}

LLVMValueRef cast(Value *source, Type *to) {
  Type *src = source->get_type();
  if (src->eq(to))
    return LLVMBuildBitCast(curr_builder, source->gen_val(), to->llvm_type(),
                            UN);
  if (NumType *num = dynamic_cast<NumType *>(src))
    return gen_num_cast(source->gen_val(), num, to);
  if (PointerType *ptr = dynamic_cast<PointerType *>(src))
    return gen_ptr_cast(source->gen_val(), ptr, to);
  if (StringType *str = dynamic_cast<StringType *>(src))
    return gen_str_cast(source, to);
  if (ArrayType *tup = dynamic_cast<ArrayType *>(src))
    return gen_arr_cast(source, tup, to);
  error("Invalid cast from " + src->stringify() + " to " + to->stringify());
}

class CastValue : public Value {
public:
  Value *source;
  Type *to;
  CastValue(Value *source, Type *to) : source(source), to(to) {}
  Type *get_type() { return to; }
  LLVMValueRef gen_val() { return cast(source, to); }
  LLVMValueRef gen_ptr() { error("Can't get the pointer to a cast"); }
  bool has_ptr() { return false; }
};
CastValue *Value::cast_to(Type *to) { return new CastValue(this, to); }

class NamedValue : public Value {
public:
  Value *val;
  std::string name;
  NamedValue(Value *val, std::string name) : val(val), name(name) {}
  Type *get_type() { return val->get_type(); }
  LLVMValueRef gen_val() {
    LLVMValueRef value = val->gen_val();
    LLVMSetValueName2(value, name.c_str(), name.size());
    return value;
  }
  LLVMValueRef gen_ptr() {
    LLVMValueRef value = val->gen_ptr();
    LLVMSetValueName2(value, name.c_str(), name.size());
    return value;
  }
  bool has_ptr() { return val->has_ptr(); }
};