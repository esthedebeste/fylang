#pragma once
#include "../../types.h"

/// StringExprAST - Expression class for multiple chars ("hello")
template <typename CharT> class StringExprAST : public ExprAST {
  inline static NumType char_type{NumType(sizeof(CharT) * 8, false, false)};
  ArrayType t_type;
  std::basic_string<CharT> str;

public:
  StringExprAST(std::basic_string<CharT> str)
      : str(str), t_type(&char_type, str.size()) {}
  Type *get_type() { return &t_type; }
  Value *gen_value() {
    LLVMValueRef *vals = new LLVMValueRef[str.size()];
    for (size_t i = 0; i < str.size(); i++)
      vals[i] = LLVMConstInt(char_type.llvm_type(), str[i], false);
    auto array = LLVMConstArray(char_type.llvm_type(), vals, str.size());
    return new ConstValue(&t_type, array);
  }
  bool is_constant() { return true; }
};

template <typename CharT> class PtrStringExprAST : public ExprAST {
  inline static NumType char_type{NumType(sizeof(CharT) * 8, false, false)};
  ArrayType t_type;
  PointerType p_type;

public:
  std::basic_string<CharT> str;
  bool null_terminated;
  PtrStringExprAST(std::basic_string<CharT> str, bool null_terminated)
      : str(str), t_type(&char_type, str.size() + null_terminated),
        p_type(&this->t_type), null_terminated(null_terminated) {}
  Type *get_type() { return &p_type; }
  Value *gen_value() {
    auto len = str.size();
    LLVMValueRef *vals = new LLVMValueRef[len + null_terminated];
    for (size_t i = 0; i < len; i++)
      vals[i] = LLVMConstInt(char_type.llvm_type(), str[i], false);
    if (null_terminated)
      vals[len] = LLVMConstNull(char_type.llvm_type());
    auto array =
        LLVMConstArray(char_type.llvm_type(), vals, len + null_terminated);
    LLVMValueRef ptr = LLVMAddGlobal(curr_module, t_type.llvm_type(),
                                     null_terminated ? ".c_str" : ".str");
    LLVMSetInitializer(ptr, array);
    LLVMSetGlobalConstant(ptr, true);
    LLVMSetLinkage(ptr, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(ptr, LLVMGlobalUnnamedAddr);
    LLVMSetAlignment(ptr, sizeof(CharT));
    LLVMValueRef cast =
        LLVMBuildBitCast(curr_builder, ptr, p_type.llvm_type(), UN);
    return new ConstValue(&p_type, cast);
  }
  bool is_constant() { return true; }
};