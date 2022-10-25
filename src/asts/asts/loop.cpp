#include "loop.h"
#include "null.h"

std::vector<LoopState> loop_stack;

ContinueExprAST::ContinueExprAST() {}
Type *ContinueExprAST::get_type() { return &null_type; }
Value *ContinueExprAST::gen_value() {
  if (loop_stack.empty())
    error("continue statement outside of loop.");
  auto &loop = loop_stack.back();
  LLVMBuildBr(curr_builder, loop.continue_block);
  // create a new block for unused code after continue
  LLVMPositionBuilderAtEnd(
      curr_builder,
      LLVMAppendBasicBlock(
          LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder)), UN));
  return null_value();
}

BreakExprAST::BreakExprAST() {}
Type *BreakExprAST::get_type() { return &null_type; }
Value *BreakExprAST::gen_value() {
  if (loop_stack.empty())
    error("break statement outside of loop.");
  auto &loop = loop_stack.back();
  LLVMBuildBr(curr_builder, loop.break_block);
  // create a new block for unused code after break
  LLVMPositionBuilderAtEnd(
      curr_builder,
      LLVMAppendBasicBlock(
          LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder)), UN));
  return null_value();
}

WhileExprAST::WhileExprAST(ExprAST *cond, ExprAST *body, ExprAST *elze)
    : cond(cond), body(body), elze(elze) {}
Type *WhileExprAST::get_type() { return &null_type; }
Value *WhileExprAST::gen_value() {
  // cast to bool
  LLVMValueRef cond_v1 =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
  LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
  LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  loop_stack.push_back(LoopState{
      .break_block = merge_bb,
      .continue_block = body_bb,
  });
  // while
  LLVMBuildCondBr(curr_builder, cond_v1, body_bb, else_bb);
  // body
  LLVMPositionBuilderAtEnd(curr_builder, body_bb);
  body->gen_value();
  // cast to bool
  LLVMValueRef cond_v2 =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMBuildCondBr(curr_builder, cond_v2, body_bb, merge_bb);
  // else
  LLVMAppendExistingBasicBlock(func, else_bb);
  LLVMPositionBuilderAtEnd(curr_builder, else_bb);
  if (elze)
    elze->gen_value();
  LLVMBuildBr(curr_builder, merge_bb);
  // merge
  LLVMAppendExistingBasicBlock(func, merge_bb);
  LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
  return null_value();
}

ForExprAST::ForExprAST(ExprAST *init, ExprAST *cond, ExprAST *body,
                       ExprAST *post, ExprAST *elze)
    : init(init), cond(cond), body(body), post(post), elze(elze) {}

Type *ForExprAST::get_type() { return &null_type; }
Value *ForExprAST::gen_value() {
  init->gen_value(); // let i = 0
  // cast to bool
  LLVMValueRef cond_v1 =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));
  LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, UN);
  LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  LLVMBasicBlockRef post_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, UN);
  loop_stack.push_back(LoopState{
      .break_block = merge_bb,
      .continue_block = post_bb,
  });
  // for
  LLVMBuildCondBr(curr_builder, cond_v1, body_bb, else_bb);
  // body
  LLVMPositionBuilderAtEnd(curr_builder, body_bb);
  body->gen_value(); // x[i] = 3
  LLVMBuildBr(curr_builder, post_bb);
  LLVMAppendExistingBasicBlock(func, post_bb);
  LLVMPositionBuilderAtEnd(curr_builder, post_bb);
  post->gen_value(); // i = i + 1
  // cast to bool
  LLVMValueRef cond_v2 =
      cond->gen_value()->cast_to(new NumType(1, false, false))->gen_val();
  LLVMBuildCondBr(curr_builder, cond_v2, body_bb, merge_bb);
  // else
  LLVMAppendExistingBasicBlock(func, else_bb);
  LLVMPositionBuilderAtEnd(curr_builder, else_bb);
  if (elze)
    elze->gen_value();
  LLVMBuildBr(curr_builder, merge_bb);
  // merge
  LLVMAppendExistingBasicBlock(func, merge_bb);
  LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
  return null_value();
}