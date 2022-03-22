#include "utils.cpp"
LLVMValueRef *used_globals = new LLVMValueRef[1];
size_t used_globals_len = 0;
void add_used_global(LLVMValueRef global) {
  static size_t allocated = 1;
  if (used_globals_len >= allocated) {
    allocated *= 2;
    used_globals = realloc_arr<LLVMValueRef>(used_globals, allocated);
  }
  used_globals[used_globals_len++] = global;
}
bool is_global_used(LLVMValueRef global) {
  for (size_t i = 0; i < used_globals_len; i++)
    if (global == used_globals[i])
      return true;
  return false;
}

void mark_used_globals(LLVMValueRef entry) {
  if (is_global_used(entry))
    return;
  add_used_global(entry);
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
      if (DEBUG)
        fprintf(stderr, "Removing %s\n", LLVMGetValueName(func));
      remove(func);
    }
    func = nxt;
  }
}

// unused code removal - removes all unused globals and functions from a certain
// entry point (often `fun main`)
void remove_unused_globals(LLVMModuleRef module, LLVMValueRef entryPoint) {
  used_globals_len = 0;
  mark_used_globals(entryPoint);
  loop_and_delete(module, LLVMGetFirstFunction, LLVMGetNextFunction,
                  LLVMDeleteFunction);
  loop_and_delete(module, LLVMGetFirstGlobal, LLVMGetNextGlobal,
                  LLVMDeleteGlobal);
}