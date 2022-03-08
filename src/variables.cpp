#include "consts.cpp"
#include "types.cpp"
/// Variable - Base class for variable info.
class Variable {
public:
  virtual LLVMValueRef gen_load() = 0;
  virtual LLVMValueRef gen_ptr() = 0;
};
/// ConstVariable - For when gen_load always returns the same thing.
class ConstVariable : public Variable {
public:
  LLVMValueRef const_load;
  LLVMValueRef const_ptr;
  ConstVariable(LLVMValueRef const_load, LLVMValueRef const_ptr)
      : const_load(const_load), const_ptr(const_ptr) {}
  LLVMValueRef gen_load() {
    if (!const_load)
      error("No const load set");
    return const_load;
  };
  LLVMValueRef gen_ptr() {
    if (!const_ptr)
      error("No const ptr set");
    return const_ptr;
  };
};
/// BasicLoadVariable - generates a load op.
class BasicLoadVariable : public Variable {
public:
  LLVMValueRef variable;
  Type *type;
  BasicLoadVariable(LLVMValueRef variable, Type *type)
      : variable(variable), type(type) {}
  LLVMValueRef gen_load() {
    return LLVMBuildLoad2(curr_builder, type->llvm_type(), variable, "");
  };
  LLVMValueRef gen_ptr() { return variable; };
};