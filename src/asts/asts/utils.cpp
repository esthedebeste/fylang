#include "../asts.h"
#include "../functions.h"
#include "../types.h"
#include "sizeof.h"
#include <string>
Value *build_malloc(Type *type) {
  if (auto func = get_function(std::string("malloc")))
    return func->gen_call({SizeofExprAST(type_ast(type)).gen_value()})
        ->cast_to(type->ptr());
  else
    error("malloc not defined before using 'new', maybe add 'include "
          "\"c/stdlib\"'?");
}

#include <unordered_set>
#include <vector>
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