#include "utils.h"

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
    for (size_t i = 0, c = LLVMGetNumOperands(entry); i < c; i++)
      mark_used_globals(LLVMGetOperand(entry, i));
}

std::unordered_set<LLVMValueRef> removed_globals;
inline void loop_and_delete(LLVMModuleRef module,
                            LLVMValueRef (*first)(LLVMModuleRef mod),
                            LLVMValueRef (*next)(LLVMValueRef last),
                            void (*remove)(LLVMValueRef removed)) {
  LLVMValueRef curr = first(module);
  while (curr != NULL) {
    LLVMValueRef nxt = next(curr);
    if (!is_global_used(curr)) {
      debug_log("Removing " << LLVMGetValueName(curr));
      remove(curr);
      removed_globals.insert(curr);
    }
    curr = nxt;
  }
}

// unused code removal - removes all unused globals.
void remove_unused_globals(LLVMModuleRef module,
                           std::vector<LLVMValueRef> entryPoints) {
  used_globals.clear();
  removed_globals.clear();
  for (auto &entry : entryPoints)
    mark_used_globals(entry);
  loop_and_delete(module, LLVMGetLastGlobal, LLVMGetPreviousGlobal,
                  LLVMDeleteGlobal);
}