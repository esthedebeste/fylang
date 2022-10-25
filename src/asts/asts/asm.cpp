#include "asm.h"

ASMExprAST::ASMExprAST(TypeAST *type_ast, std::string asm_str,
                       std::string out_reg,
                       std::vector<std::pair<std::string, ExprAST *>> args)
    : type_ast(type_ast), asm_str(asm_str), has_output(true), out_reg(out_reg),
      args(args) {}
ASMExprAST::ASMExprAST(std::string asm_str,
                       std::vector<std::pair<std::string, ExprAST *>> args)
    : asm_str(asm_str), has_output(false), args(args) {}

Type *ASMExprAST::get_type() {
  return has_output ? type_ast->type() : new NullType();
}
Value *ASMExprAST::gen_value() {
  LLVMValueRef *arg_vs = new LLVMValueRef[args.size()];
  LLVMTypeRef *arg_ts = new LLVMTypeRef[args.size()];
  std::vector<std::string> constraints;
  if (has_output)
    constraints.push_back("={" + out_reg + "}");
  for (size_t i = 0; i < args.size(); i++) {
    arg_vs[i] = args[i].second->gen_value()->gen_val();
    arg_ts[i] = args[i].second->get_type()->llvm_type();
    constraints.push_back("{" + args[i].first + "}");
  }
  std::string constraints_str = "";
  for (size_t i = 0; i < constraints.size(); i++) {
    if (i != 0)
      constraints_str += ",";
    constraints_str += constraints[i];
  }
  Type *type = has_output ? type_ast->type() : new NullType();
  LLVMTypeRef functy =
      LLVMFunctionType(has_output ? type->llvm_type() : LLVMVoidType(), arg_ts,
                       args.size(), false);
  LLVMValueRef inline_asm =
      LLVMGetInlineAsm(functy, asm_str.data(), asm_str.size(),
                       constraints_str.data(), constraints_str.size(),
                       /* has side effects */ !has_output, false,
                       LLVMInlineAsmDialectATT, false);
  LLVMValueRef call = LLVMBuildCall2(curr_builder, functy, inline_asm, arg_vs,
                                     args.size(), has_output ? UN : "");
  return new ConstValue(
      type, has_output ? call : LLVMConstNull(NullType().llvm_type()));
}

GlobalASMExprAST::GlobalASMExprAST(std::string asm_str) : asm_str(asm_str) {}
void GlobalASMExprAST::gen_toplevel() {
  LLVMAppendModuleInlineAsm(curr_module, asm_str.data(), asm_str.size());
}