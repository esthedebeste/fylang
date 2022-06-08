#include "../asts.h"

OrExprAST::OrExprAST(ExprAST *left, ExprAST *right)
    : left(left), right(right) {}

Type *OrExprAST::get_type() {
  auto left_t = left->get_type();
  auto right_t = right->get_type();
  if (left_t->neq(right_t))
    error("and/or's left and right side don't have the same type, " +
          left_t->stringify() + " does not match " + right_t->stringify() +
          ".");
  return left_t;
}

enum Shortcircuit { Or, And };
Value *gen_shortcircuit(Type *type, ExprAST *lefte, ExprAST *righte,
                        Shortcircuit sc_type) {
  auto left_bb = LLVMGetInsertBlock(curr_builder);
  auto left = lefte->gen_value();
  // cast to bool
  auto left_bool = left->cast_to(new NumType(1, false, false))->gen_val();
  left_bb = LLVMGetInsertBlock(curr_builder);
  auto func = LLVMGetBasicBlockParent(left_bb);
  auto right_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
  auto merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  if (sc_type == Or) {
    // if left is true, skip right
    LLVMBuildCondBr(curr_builder, left_bool, merge_bb, right_bb);
  } else /* sc_type == And */ {
    // if left is false, skip right
    LLVMBuildCondBr(curr_builder, left_bool, right_bb, left_bb);
  }
  LLVMPositionBuilderAtEnd(curr_builder, right_bb);
  auto right = righte->gen_value();
  LLVMBuildBr(curr_builder, merge_bb);
  // right can change the current block, update then_bb for the PHI.
  right_bb = LLVMGetInsertBlock(curr_builder);
  // merge
  LLVMAppendExistingBasicBlock(func, merge_bb);
  LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
  return gen_phi(left_bb, left, right_bb, right);
}

Value *OrExprAST::gen_value() {
  return gen_shortcircuit(get_type(), left, right, Or);
}

Value *AndExprAST::gen_value() {
  return gen_shortcircuit(get_type(), left, right, And);
}

void IfExprAST::init() {
  Type *then_t = then->get_type();
  type = then_t;
  if (null_else)
    elze = new NullExprAST(type);
  Type *else_t = elze->get_type();
  if (then_t->neq(else_t))
    error("conditional's then and else side don't have the same type, " +
          then_t->stringify() + " does not match " + else_t->stringify() + ".");
}
IfExprAST::IfExprAST(ExprAST *cond, ExprAST *then,
                     // elze because else cant be a variable name lol
                     ExprAST *elze)
    : cond(cond), then(then), elze(elze), null_else(elze == nullptr) {}

Type *IfExprAST::get_type() {
  init();
  return type;
}

Value *IfExprAST::gen_value() {
  init();
  // cast to bool
  LLVMValueRef cond_v =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
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
  return gen_phi(then_bb, then_v, else_bb, else_v);
}
