#pragma once
#include "../asts.h"

Value *build_malloc(Type *type);
void add_store_before_main(LLVMValueRef ptr, ExprAST *val);
void add_stores_before_main(LLVMValueRef main_func);