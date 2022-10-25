#pragma once
#include "../asts.h"
#include "../types.h"
#include <string>
#include <vector>

/// ASMExprAST - Inline assembly
class ASMExprAST : public ExprAST {
  TypeAST *type_ast;
  std::string asm_str;
  bool has_output;
  std::string out_reg;
  std::vector<std::pair<std::string, ExprAST *>> args;

public:
  ASMExprAST(TypeAST *type_ast, std::string asm_str, std::string out_reg,
             std::vector<std::pair<std::string, ExprAST *>> args);
  ASMExprAST(std::string asm_str,
             std::vector<std::pair<std::string, ExprAST *>> args);

  Type *get_type();
  Value *gen_value();
};

/// GlobalASMExprAST - Module-level inline assembly
class GlobalASMExprAST {
  std::string asm_str;

public:
  GlobalASMExprAST(std::string asm_str);
  void gen_toplevel();
};