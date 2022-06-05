#include "../asts.h"

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
  NumType uint_ptrsize(false);
  auto il = LLVMBuildPtrToInt(curr_builder, L, uint_ptrsize.llvm_type(), UN);
  auto ir = LLVMBuildPtrToInt(curr_builder, R, uint_ptrsize.llvm_type(), UN);
  auto result = gen_num_num_binop(op, il, ir, &uint_ptrsize, &uint_ptrsize);
  if (binop_precedence[op] == comparison_prec)
    return result;
  else
    return LLVMBuildIntToPtr(curr_builder, result, lhs_ptr->llvm_type(), UN);
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
  LLVMValueRef res = nullptr;
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
  else if (lhs_tt == Pointer && rhs_tt == Pointer)
    return lhs_t; // ptr + ptr returns ptr
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

BinaryExprAST::BinaryExprAST(int op, ExprAST *LHS, ExprAST *RHS)
    : op(op), LHS(LHS), RHS(RHS) {}

Type *BinaryExprAST::get_type() {
  return get_binop_type(op, LHS->get_type(), RHS->get_type());
}

Value *BinaryExprAST::gen_value() {
  return gen_binop(op, LHS->gen_value()->gen_val(), RHS->gen_value()->gen_val(),
                   LHS->get_type(), RHS->get_type());
}
// LLVM can constantify binary expressions if both sides are also constant.
bool BinaryExprAST::is_constant() {
  return LHS->is_constant() && RHS->is_constant();
}