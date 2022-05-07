#pragma once
#include "asts.cpp"
#include "types.cpp"

struct FuncFlags {
  bool is_vararg = false, is_inline = false;
};
struct ReturnState {
  Type *return_type;
  LLVMBasicBlockRef return_block;
  LLVMValueRef return_phi;
};
static ReturnState curr_return_state;
void add_return(LLVMValueRef ret_val, LLVMBasicBlockRef curr_block =
                                          LLVMGetInsertBlock(curr_builder)) {
  LLVMBuildBr(curr_builder, curr_return_state.return_block);
  LLVMAddIncoming(curr_return_state.return_phi, &ret_val, &curr_block, 1);
}
void add_return(Value *ret_val, LLVMBasicBlockRef curr_block =
                                    LLVMGetInsertBlock(curr_builder)) {
  return add_return(ret_val->cast_to(curr_return_state.return_type)->gen_val(),
                    curr_block);
}

class MethodAST;
static std::unordered_map<std::string, std::vector<MethodAST *>>
    curr_extension_methods;

class FunctionAST;
std::unordered_map<std::string, FunctionAST *> curr_named_functions;
class FunctionAST {
public:
  std::string name;
  std::vector<std::pair<std::string, TypeAST *>> args;
  ExprAST *body;
  FunctionTypeAST ft;
  FuncFlags flags;
  FunctionAST(std::string name,
              std::vector<std::pair<std::string, TypeAST *>> args,
              FuncFlags flags = {}, TypeAST *return_type = nullptr,
              ExprAST *body = nullptr)
      : name(name), args(args), flags(flags), body(body),
        ft(return_type, seconds(args), flags.is_vararg) {}
  std::unordered_map<FunctionType *, FuncValue *> already_declared;
  virtual std::string get_name(FunctionType *type) {
    if (!ft.is_generic())
      return name;
    std::stringstream str;
    str << name << "<";
    size_t c = 0;
    for (size_t i = 0; i < type->arguments.size(); i++) {
      if (ft.args[i]->is_generic()) {
        if (c != 0)
          str << ", ";
        str << type->arguments[i]->stringify();
        c++;
      }
    }
    str << ">";
    return str.str();
  }
  FuncValue *declare(FunctionType *type) {
    std::string name = get_name(type);
    if (already_declared.count(type))
      return already_declared[type];
    if (auto func = LLVMGetNamedFunction(curr_module, name.c_str()))
      return already_declared[type] = new FuncValue(type, func);
    LLVMValueRef func =
        LLVMAddFunction(curr_module, name.c_str(), type->llvm_type());
    already_declared[type] = new FuncValue(type, func);
    return new FuncValue(type, func);
  }
  FunctionType *get_type() {
    push_scope();
    for (auto &[name, type] : this->args)
      curr_scope->declare_variable(name, type->type());
    FunctionType *type = ft.func_type();
    if (!type->return_type && body)
      type->return_type = body->get_type();
    if (!type->return_type)
      error("can't infer return type of function " + name);
    pop_scope();
    return type;
  }
  FunctionType *get_type(std::vector<ExprAST *> args) {
    if (ft.vararg ? args.size() < this->args.size()
                  : args.size() != this->args.size())
      error("wrong number of arguments to function "
            << name << " (expected " << this->args.size() << ", got "
            << args.size() << ")");
    for (size_t i = 0; i < args.size(); ++i) {
      Type *arg_type = args[i]->get_type();
      if (i < this->args.size() &&
          !this->args[i].second->castable_from(arg_type))
        error("argument " + std::to_string(i) + " to function " + name +
              " has wrong type, got: " + arg_type->stringify() +
              ", expected: " + this->args[i].second->stringify());
    }
    return get_type();
  }
  // Returns PHI of return value, moves to return block
  LLVMValueRef gen_body(LLVMValueRef *args, FunctionType *type) {
    for (size_t i = 0; i < this->args.size(); i++)
      curr_scope->set_variable(
          this->args[i].first,
          new NamedValue(new ConstValue(type->arguments[i], args[i]),
                         this->args[i].first));
    ReturnState prev_return_state = curr_return_state;
    LLVMBasicBlockRef body_bb = LLVMGetInsertBlock(curr_builder);
    LLVMBasicBlockRef ret_bb =
        LLVMAppendBasicBlock(LLVMGetBasicBlockParent(body_bb), UN);
    LLVMPositionBuilderAtEnd(curr_builder, ret_bb);
    LLVMValueRef ret_phi =
        LLVMBuildPhi(curr_builder, type->return_type->llvm_type(), UN);
    LLVMPositionBuilderAtEnd(curr_builder, body_bb);
    curr_return_state = {type->return_type, ret_bb, ret_phi};
    Value *body_val = body->gen_value();
    add_return(body_val);
    LLVMPositionBuilderAtEnd(curr_builder, ret_bb);
    curr_return_state = prev_return_state;
    return ret_phi;
  }
  ConstValue *gen_call(std::vector<ExprAST *> args) {
    if (ft.vararg ? args.size() < this->args.size()
                  : args.size() != this->args.size())
      error("wrong number of arguments to function "
            << name << " (expected " << this->args.size() << ", got "
            << args.size() << ")");
    std::vector<Value *> arg_vals(args.size());
    for (size_t i = 0; i < args.size(); ++i)
      arg_vals[i] = args[i]->gen_value();
    Scope *prev_scope = curr_scope;
    curr_scope = new Scope(global_scope);
    for (size_t i = 0; i < args.size(); ++i) {
      Type *arg_type = arg_vals[i]->get_type();
      if (i < this->args.size()) {
        if (this->args[i].second->castable_from(arg_type))
          arg_vals[i] = arg_vals[i]->cast_to(this->args[i].second->type());
        else
          error("argument " + std::to_string(i) + " to function " + name +
                " has wrong type, got: " + arg_type->stringify() +
                ", expected: " + this->args[i].second->stringify());
      }
    }
    LLVMValueRef *llvm_args = new LLVMValueRef[arg_vals.size()];
    for (size_t i = 0; i < arg_vals.size(); ++i)
      llvm_args[i] = arg_vals[i]->gen_val();
    FunctionType *type = ft.func_type();
    for (auto &[name, type] : this->args)
      curr_scope->declare_variable(name, type->type());

    if (!type->return_type) {
      if (body)
        type->return_type = body->get_type();
      else
        error("can't infer return type of function " + name);
    }
    if (flags.is_inline) {
      debug_log("inlining " << name);
      LLVMValueRef ret = gen_body(llvm_args, type);
      curr_scope = prev_scope;
      return new ConstValue(type->return_type, ret);
    }
    FuncValue *declaration = declare(type);
    LLVMValueRef call =
        LLVMBuildCall2(curr_builder, type->llvm_type(), declaration->func,
                       llvm_args, arg_vals.size(), ("call_" + name).c_str());
    if (body && !LLVMGetFirstBasicBlock(declaration->func)) {
      LLVMSetFunctionCallConv(declaration->func, LLVMFastCallConv);
      LLVMSetLinkage(declaration->func, LLVMInternalLinkage);
      size_t prev_unnamed = unnamed_acc;
      unnamed_acc = 0;
      LLVMBasicBlockRef block = LLVMAppendBasicBlock(declaration->func, "");
      LLVMPositionBuilderAtEnd(curr_builder, block);
      // reuse llvm_args
      LLVMGetParams(declaration->func, llvm_args);
      LLVMValueRef ret = gen_body(llvm_args, type);
      LLVMBuildRet(curr_builder, ret);
      unnamed_acc = prev_unnamed;
      if (auto next = LLVMGetNextInstruction(call))
        LLVMPositionBuilderBefore(curr_builder, next);
      else
        LLVMPositionBuilderAtEnd(curr_builder, LLVMGetInstructionParent(call));
    }
    curr_scope = prev_scope;
    return new ConstValue(type->return_type, call);
  }

  FuncValue *gen_ptr() {
    if (ft.vararg ? args.size() < this->args.size()
                  : args.size() != this->args.size())
      error("wrong number of arguments to function "
            << name << " (expected " << this->args.size() << ", got "
            << args.size() << ")");
    Scope *prev_scope = curr_scope;
    curr_scope = new Scope(global_scope);
    for (auto &[name, type] : this->args)
      curr_scope->declare_variable(name, type->type());

    FunctionType *type = ft.func_type();
    if (!type->return_type) {
      if (body)
        type->return_type = body->get_type();
      else
        error("can't infer return type of function " + name);
    }
    FuncValue *declaration = declare(type);
    if (body && !LLVMGetFirstBasicBlock(declaration->func)) {
      LLVMValueRef position_back_to =
          LLVMGetInsertBlock(curr_builder)
              ? LLVMBuildAlloca(curr_builder, VoidType().llvm_type(), UN)
              : nullptr;
      size_t prev_unnamed = unnamed_acc;
      unnamed_acc = 0;
      LLVMBasicBlockRef block = LLVMAppendBasicBlock(declaration->func, "");
      LLVMPositionBuilderAtEnd(curr_builder, block);
      LLVMValueRef *llvm_args = new LLVMValueRef[this->args.size()];
      LLVMGetParams(declaration->func, llvm_args);
      LLVMValueRef ret = gen_body(llvm_args, type);
      LLVMBuildRet(curr_builder, ret);
      unnamed_acc = prev_unnamed;
      if (position_back_to) {
        if (auto next = LLVMGetNextInstruction(position_back_to))
          LLVMPositionBuilderBefore(curr_builder, next);
        else
          LLVMPositionBuilderAtEnd(curr_builder,
                                   LLVMGetInstructionParent(position_back_to));
        LLVMInstructionEraseFromParent(position_back_to);
      }
    }
    curr_scope = prev_scope;
    return declaration;
  }

  virtual void add() { curr_named_functions[name] = this; }
};

auto add_this_type(std::vector<std::pair<std::string, TypeAST *>> args,
                   TypeAST *type) {
  args.push_back(std::make_pair("this", type));
  return args;
}

class MethodAST : public FunctionAST {
public:
  TypeAST *this_type;
  MethodAST(TypeAST *this_type, std::string name,
            std::vector<std::pair<std::string, TypeAST *>> args,
            FuncFlags flags = {}, TypeAST *return_type = nullptr,
            ExprAST *body = nullptr)
      : FunctionAST(name, add_this_type(args, this_type), flags, return_type,
                    body),
        this_type(this_type) {}
  std::string get_name(FunctionType *type) {
    return this_type->type()->stringify() + "." + FunctionAST::get_name(type);
  }
  FunctionType *get_type(std::vector<ExprAST *> args, ExprAST *this_arg) {
    args.push_back(this_arg);
    return FunctionAST::get_type(args);
  }
  Value *gen_call(std::vector<ExprAST *> args, ExprAST *this_arg) {
    args.push_back(this_arg);
    return FunctionAST::gen_call(args);
  }

  virtual void add() { curr_extension_methods[name].push_back(this); }
};

MethodAST *get_method(Type *this_type, std::string name) {
  if (!curr_extension_methods.count(name))
    return nullptr;
  uint min_generic = UINT_MAX;
  MethodAST *best_match = nullptr;
  for (auto &method : curr_extension_methods[name]) {
    uint generic_count = 0;
    if (method->this_type->match(this_type, &generic_count) &&
        generic_count < min_generic) {
      min_generic = generic_count, best_match = method;
    }
  }
  return best_match;
}