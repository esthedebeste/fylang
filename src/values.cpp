#include "values.h"

ConstValue::ConstValue(Type *type, LLVMValueRef val) : type(type), val(val) {}
Type *ConstValue::get_type() { return type; };
LLVMValueRef ConstValue::gen_val() { return val; };
LLVMValueRef ConstValue::gen_ptr() {
  error("Const values can't be pointered");
};
bool ConstValue::has_ptr() { return false; }
bool ConstValue::is_constant() { return LLVMIsConstant(val); }

ConstValueWithPtr::ConstValueWithPtr(Type *type, LLVMValueRef ptr,
                                     LLVMValueRef val)
    : type(type), ptr(ptr), val(val) {}
Type *ConstValueWithPtr::get_type() { return type; };
LLVMValueRef ConstValueWithPtr::gen_val() { return val; };
LLVMValueRef ConstValueWithPtr::gen_ptr() { return ptr; };
bool ConstValueWithPtr::has_ptr() { return true; }
bool ConstValueWithPtr::is_constant() {
  return LLVMIsConstant(val) && LLVMIsConstant(ptr);
}

IntValue::IntValue(NumType type, uint64_t val) : type(type), val(val) {}
Type *IntValue::get_type() { return &type; };
LLVMValueRef IntValue::gen_val() {
  return LLVMConstInt(type.llvm_type(), val, type.is_signed);
};
LLVMValueRef IntValue::gen_ptr() { error("Int values can't be pointered"); };
bool IntValue::has_ptr() { return false; }
bool IntValue::is_constant() { return true; }

FuncValue::FuncValue(Type *type, LLVMValueRef func) : type(type), func(func) {}
Type *FuncValue::get_type() { return type; };
LLVMValueRef FuncValue::gen_val() { return func; };
LLVMValueRef FuncValue::gen_ptr() { return func; };
bool FuncValue::has_ptr() { return true; }
bool FuncValue::is_constant() { return true; }

BasicLoadValue::BasicLoadValue(Type *type, LLVMValueRef variable)
    : type(type), variable(variable) {}
Type *BasicLoadValue::get_type() { return type; }
LLVMValueRef BasicLoadValue::gen_val() {
  return LLVMBuildLoad2(curr_builder, type->llvm_type(), variable, UN);
};
LLVMValueRef BasicLoadValue::gen_ptr() { return variable; };
bool BasicLoadValue::has_ptr() { return true; }
bool BasicLoadValue::is_constant() { return false; }

ConstValue *gen_phi(LLVMBasicBlockRef a_bb, Value *a_v, LLVMBasicBlockRef b_bb,
                    Value *b_v) {
  if (a_v->get_type()->neq(b_v->get_type()))
    error("conditional's values must have the same type");
  Type *type = a_v->get_type();
  LLVMBasicBlockRef curr = LLVMGetInsertBlock(curr_builder);
  LLVMPositionBuilderBefore(
      curr_builder, LLVMGetLastInstruction(a_bb)); /* before the terminator */
  LLVMValueRef a_val = a_v->gen_val();
  LLVMPositionBuilderBefore(
      curr_builder, LLVMGetLastInstruction(b_bb)); /* before the terminator */
  LLVMValueRef b_val = b_v->gen_val();
  LLVMValueRef incoming_v[2] = {a_val, b_val};
  LLVMBasicBlockRef incoming_bb[2] = {a_bb, b_bb};
  LLVMPositionBuilderAtEnd(curr_builder, curr);
  LLVMValueRef load = LLVMBuildPhi(curr_builder, type->llvm_type(), UN);
  LLVMAddIncoming(load, incoming_v, incoming_bb, 2);
  return new ConstValue(type, load);
}

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
  error(a->stringify() + " can't be casted to " + b->stringify());
}

LLVMValueRef gen_ptr_cast(LLVMValueRef value, PointerType *a, Type *b) {
  if (b->type_type() == TypeType::Pointer)
    return LLVMBuildPointerCast(curr_builder, value, b->llvm_type(), UN);
  else if (NumType *num = dynamic_cast<NumType *>(b)) {
    if (num->bits != 1)
      return LLVMBuildPtrToInt(curr_builder, value, b->llvm_type(), UN);
    else /* x != 0 */
      return LLVMBuildICmp(curr_builder, LLVMIntPredicate::LLVMIntNE, value,
                           LLVMConstNull(a->llvm_type()), UN);
  }
  error(a->stringify() + " can't be casted to " + b->stringify());
}

LLVMValueRef gen_arr_cast(Value *value, ArrayType *a, Type *b) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(b)) {
    if (ptr->get_points_to()->neq(a->get_elem_type()))
      error("Array can't be casted to pointer with different type, " +
            a->get_elem_type()->stringify() + "[" + std::to_string(a->count) +
            "] can't be casted to *" + ptr->get_points_to()->stringify() + ".");
    if (!value->has_ptr())
      error("const arrays can't be automatically casted to a pointer to their "
            "elements.");
    LLVMValueRef zeros[2] = {
        LLVMConstInt(NumType(false).llvm_type(), 0, false),
        LLVMConstInt(NumType(false).llvm_type(), 0, false)};
    // cast [ ... x T ]* to T*
    LLVMValueRef cast = LLVMBuildGEP2(curr_builder, a->llvm_type(),
                                      value->gen_ptr(), zeros, 2, UN);
    return cast;
  }
  error(a->stringify() + " can't be casted to " + b->stringify());
}

LLVMValueRef gen_tuple_cast(Value *value, TupleType *a, Type *b) {
  if (ArrayType *arr = dynamic_cast<ArrayType *>(b)) {
    auto elem_type = arr->elem;
    for (Type *member : a->types)
      if (member->neq(elem_type))
        error("Tuple can't be casted to array with different type, " +
              a->stringify() + " can't be casted to " + arr->stringify() + ".");
    if (arr->count != a->types.size())
      error("Tuple can't be casted to array with different size, " +
            a->stringify() + " can't be casted to " + arr->stringify() + ".");
    if (value->has_ptr()) {
      // load (A, A, A) as [A x 3]
      return LLVMBuildLoad2(curr_builder, arr->llvm_type(),
                            LLVMBuildBitCast(curr_builder, value->gen_ptr(),
                                             arr->llvm_type(), UN),
                            UN);
    } else {
      LLVMValueRef arr_v = LLVMGetUndef(arr->llvm_type());
      auto tup_v = value->gen_val();
      for (size_t i = 0; i < a->types.size(); i++)
        arr_v = LLVMBuildInsertValue(
            curr_builder, arr_v,
            LLVMBuildExtractValue(curr_builder, tup_v, i, UN), i, UN);
      return arr_v;
    }
  }
  error(a->stringify() + " can't be casted to " + b->stringify());
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
  if (ArrayType *arr = dynamic_cast<ArrayType *>(src))
    return gen_arr_cast(source, arr, to);
  if (TupleType *tup = dynamic_cast<TupleType *>(src))
    return gen_tuple_cast(source, tup, to);
  if (src->type_type() == TypeType::Null)
    return LLVMConstNull(to->llvm_type());
  error("Invalid cast from " + src->stringify() + " to " + to->stringify());
}

CastValue::CastValue(Value *source, Type *to) : source(source), to(to) {}
Type *CastValue::get_type() { return to; }
LLVMValueRef CastValue::gen_val() { return cast(source, to); }
LLVMValueRef CastValue::gen_ptr() { error("Can't get the pointer to a cast"); }
bool CastValue::has_ptr() { return false; }
bool CastValue::is_constant() { return source->is_constant(); }

CastValue *Value::cast_to(Type *to) { return new CastValue(this, to); }