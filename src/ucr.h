#pragma once
extern std::unordered_set<LLVMValueRef> removed_globals;
void remove_unused_globals(LLVMModuleRef module,
                           std::vector<LLVMValueRef> entryPoints);