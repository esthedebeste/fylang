#pragma once
#include "../asts.h"

struct LoopState {
  LLVMBasicBlockRef break_block;
  LLVMBasicBlockRef continue_block;
};
extern std::vector<LoopState> loop_stack;

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