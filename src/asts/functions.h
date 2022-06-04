#pragma once
#include "../values.h"
#include "types.h"

struct ReturnState {
  Type *return_type;
  LLVMBasicBlockRef return_block;
  LLVMValueRef return_phi;
};
extern ReturnState curr_return_state;
void add_return(LLVMValueRef ret_val, LLVMBasicBlockRef curr_block =
                                          LLVMGetInsertBlock(curr_builder));
void add_return(Value *ret_val, LLVMBasicBlockRef curr_block =
                                    LLVMGetInsertBlock(curr_builder));

class MethodAST;
class FunctionAST;
extern std::unordered_map<std::string, std::vector<MethodAST *>>
    curr_extension_methods;
extern std::unordered_map<std::string, FunctionAST *> curr_named_functions;

class ExprAST;
class FunctionAST {
public:
  std::string name;
  std::vector<std::pair<std::string, TypeAST *>> args;
  ExprAST *body;
  FunctionTypeAST ft;
  FuncFlags flags;
  std::unordered_map<FunctionType *, FuncValue *> already_declared;
  FunctionAST(std::string name,
              std::vector<std::pair<std::string, TypeAST *>> args,
              FuncFlags flags = {}, TypeAST *return_type = nullptr,
              ExprAST *body = nullptr);
  virtual std::string get_name(FunctionType *type);
  FuncValue *declare(FunctionType *type);
  FunctionType *get_type();
  FunctionType *get_type(std::vector<ExprAST *> args);
  // Returns PHI of return value, moves to return block
  LLVMValueRef gen_body(LLVMValueRef *args, FunctionType *type);
  ConstValue *gen_call(std::vector<ExprAST *> args);
  ConstValue *gen_call(std::vector<Value *> arg_vals);
  FuncValue *gen_ptr();
  virtual void add();
};

class MethodAST : public FunctionAST {
public:
  TypeAST *this_type;
  MethodAST(TypeAST *this_type, std::string name,
            std::vector<std::pair<std::string, TypeAST *>> args,
            FuncFlags flags = {}, TypeAST *return_type = nullptr,
            ExprAST *body = nullptr);
  std::string get_name(FunctionType *type) override;
  FunctionType *get_type(std::vector<ExprAST *> args, ExprAST *this_arg);
  Value *gen_call(std::vector<ExprAST *> args, ExprAST *this_arg);
  void add() override;
};

MethodAST *get_method(Type *this_type, std::string name);