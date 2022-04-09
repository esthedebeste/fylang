#include "utils.cpp"
std::vector<LLVMValueRef> used_globals;
bool is_global_used(LLVMValueRef global) {
  for (auto &used_global : used_globals)
    if (used_global == global)
      return true;
  return false;
}

void mark_used_globals(LLVMValueRef entry) {
  if (is_global_used(entry))
    return;
  used_globals.push_back(entry);
  if (LLVMIsAFunction(entry)) {
    LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(entry);
    while (block != NULL) {
      LLVMValueRef inst = LLVMGetFirstInstruction(block);
      while (inst != NULL) {
        if (LLVMIsAUser(inst))
          mark_used_globals(inst);
        inst = LLVMGetNextInstruction(inst);
      }
      block = LLVMGetNextBasicBlock(block);
    }
  }
  if (LLVMIsAUser(entry))
    for (unsigned i = 0, c = LLVMGetNumOperands(entry); i < c; i++)
      mark_used_globals(LLVMGetOperand(entry, i));
}

void loop_and_delete(LLVMModuleRef module,
                     LLVMValueRef (*first)(LLVMModuleRef mod),
                     LLVMValueRef (*next)(LLVMValueRef last),
                     void (*remove)(LLVMValueRef removed)) {
  LLVMValueRef func = first(module);
  while (func != NULL) {
    LLVMValueRef nxt = next(func);
    if (!is_global_used(func)) {
      debug_log("Removing " << LLVMGetValueName(func));
      remove(func);
    }
    func = nxt;
  }
}

// unused code removal - removes all unused globals and functions from a certain
// entry point (often `fun main`)
void remove_unused_globals(LLVMModuleRef module, LLVMValueRef entryPoint) {
  used_globals.clear();
  mark_used_globals(entryPoint);
  loop_and_delete(module, LLVMGetFirstFunction, LLVMGetNextFunction,
                  LLVMDeleteFunction);
  loop_and_delete(module, LLVMGetFirstGlobal, LLVMGetNextGlobal,
                  LLVMDeleteGlobal);
}