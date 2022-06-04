#include "asts.h"

Value *build_malloc(Type *type) {
  if (!curr_named_functions.count("malloc"))
    error("malloc not defined before using 'new', maybe add 'include "
          "\"c/stdlib\"'?");
  return new NamedValue(
      curr_named_functions["malloc"]
          ->gen_call({SizeofExprAST(type_ast(type)).gen_value()})
          ->cast_to(type->ptr()),
      "malloc_" + type->stringify());
}

ExprAST::~ExprAST() {}
bool ExprAST::is_constant() { return false; }

std::vector<std::pair<LLVMValueRef, ExprAST *>> inits;
void add_store_before_main(LLVMValueRef ptr, ExprAST *val) {
  inits.push_back({ptr, val});
}
extern std::unordered_set<LLVMValueRef> removed_globals; // defined in UCR
void add_stores_before_main(LLVMValueRef main_func) {
  if (inits.size() == 0)
    return; // nothing to do
  LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(main_func);
  LLVMBasicBlockRef store_block =
      LLVMAppendBasicBlock(main_func, "global_vars");
  LLVMMoveBasicBlockBefore(store_block, entry);
  LLVMPositionBuilderAtEnd(curr_builder, store_block);
  bool has_non_constant_init = false;
  for (auto &[ptr, expr] : inits)
    // UCR can remove globals, so we need to check if the global still exists
    if (removed_globals.count(ptr) == 0) {
      LLVMValueRef val = expr->gen_value()->gen_val();
      if (LLVMIsConstant(val))
        LLVMSetInitializer(ptr, val);
      else {
        LLVMBuildStore(curr_builder, val, ptr);
        has_non_constant_init = true;
      }
    }
  LLVMBuildBr(curr_builder, entry);
  if (!has_non_constant_init)
    LLVMDeleteBasicBlock(store_block);
}

CharExprAST::CharExprAST(char data)
    : NumberExprAST(data, NumType(8, false, false)) {}

AssignExprAST::AssignExprAST(ExprAST *LHS, ExprAST *RHS) : LHS(LHS), RHS(RHS) {}
Type *AssignExprAST::get_type() { return LHS->get_type(); }
Value *AssignExprAST::gen_value() {
  CastValue *val = RHS->gen_value()->cast_to(LHS->get_type());
  LLVMBuildStore(curr_builder, val->gen_val(), LHS->gen_value()->gen_ptr());
  return val;
}

BlockExprAST::BlockExprAST(std::vector<ExprAST *> exprs) : exprs(exprs) {
  if (exprs.size() == 0)
    error("block can't be empty.");
}
Type *BlockExprAST::get_type() {
  push_scope();
  // initialize previous exprs
  for (size_t i = 0; i < exprs.size() - 1; i++)
    exprs[i]->get_type();
  Type *type = exprs.back()->get_type();
  pop_scope();
  return type;
}
Value *BlockExprAST::gen_value() {
  push_scope();
  // generate code for all exprs and only return last expr
  for (size_t i = 0; i < exprs.size() - 1; i++)
    exprs[i]->gen_value();
  Value *value = exprs.back()->gen_value();
  pop_scope();
  return value;
}

ConstValue *null_value(Type *type) {
  return new ConstValue(type, LLVMConstNull(type->llvm_type()));
}

NullExprAST::NullExprAST(Type *type) : type(type) {}
NullExprAST::NullExprAST() : type(&null_type) {}
Type *NullExprAST::get_type() { return type; }
Value *NullExprAST::gen_value() { return null_value(type); }
bool NullExprAST::is_constant() { return true; }

DeclareExprAST::DeclareExprAST(LetExprAST *let) : let(let) {
  curr_scope->declare_variable(let->id, let->get_type());
}
DeclareExprAST::DeclareExprAST(FunctionAST *func) : func(func) {
  curr_named_functions[func->name] = func;
}
LLVMValueRef DeclareExprAST::gen_toplevel() {
  if (let)
    return let->gen_declare();
  return nullptr;
}