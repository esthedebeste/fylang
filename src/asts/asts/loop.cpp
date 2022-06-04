#include "../asts.h"

Value *WhileExprAST::gen_value() {
  init();
  // cast to bool
  LLVMValueRef cond_v =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
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
  return gen_phi(then_bb, then_v, else_bb, else_v);
}

ForExprAST::ForExprAST(ExprAST *init, ExprAST *cond, ExprAST *body,
                       ExprAST *post, ExprAST *elze)
    : init(init), post(post), IfExprAST(cond, body, elze) {}

Type *ForExprAST::get_type() {
  init->get_type();
  return IfExprAST::get_type();
}
Value *ForExprAST::gen_value() {
  init->gen_value(); // let i = 0
  IfExprAST::init();
  // cast to bool
  LLVMValueRef cond_v =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
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
  return gen_phi(then_bb, then_v, else_bb, else_v);
}