#pragma once
#include "asts.cpp"
#include "types.cpp"

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
  FunctionAST(std::string name,
              std::vector<std::pair<std::string, TypeAST *>> args, bool vararg,
              TypeAST *return_type = nullptr, ExprAST *body = nullptr)
      : name(name), args(args), body(body),
        ft(return_type, seconds(args), vararg) {}
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
    declare(type); // declare function when accessed
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
  ConstValue *gen_call(std::vector<ExprAST *> args) {
    if (ft.vararg ? args.size() < this->args.size()
                  : args.size() != this->args.size())
      error("wrong number of arguments to function "
            << name << " (expected " << this->args.size() << ", got "
            << args.size() << ")");
    std::vector<Value *> arg_vals(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
      Value *arg = args[i]->gen_value();
      Type *arg_type = arg->get_type();
      if (i < this->args.size()) {
        if (this->args[i].second->castable_from(arg_type))
          arg = arg->cast_to(this->args[i].second->type());
        else
          error("argument " + std::to_string(i) + " to function " + name +
                " has wrong type, got: " + arg_type->stringify() +
                ", expected: " + this->args[i].second->stringify());
      }
      arg_vals[i] = arg;
    }
    LLVMValueRef *vals = new LLVMValueRef[arg_vals.size()];
    for (size_t i = 0; i < arg_vals.size(); ++i)
      vals[i] = arg_vals[i]->gen_val();
    FunctionType *type = ft.func_type();
    push_scope();

    for (auto &[name, type] : this->args)
      curr_scope->declare_variable(name, type->type());

    if (!type->return_type) {
      if (body)
        type->return_type = body->get_type();
      else
        error("can't infer return type of function " + name);
    }
    FuncValue *declaration = declare(type);
    LLVMValueRef call =
        LLVMBuildCall2(curr_builder, type->llvm_type(), declaration->func, vals,
                       arg_vals.size(), ("call_" + name).c_str());
    if (body && !LLVMGetFirstBasicBlock(declaration->func)) {
      for (size_t i = 0; i < type->arguments.size(); i++)
        curr_scope->set_variable(
            this->args[i].first,
            new NamedValue(new ConstValue(type->arguments[i],
                                          LLVMGetParam(declaration->func, i)),
                           this->args[i].first));
      size_t prev_unnamed = unnamed_acc;
      unnamed_acc = 0;
      FunctionType *prev_func_type = curr_func_type;
      curr_func_type = type;
      LLVMBasicBlockRef block = LLVMAppendBasicBlock(declaration->func, "");
      LLVMPositionBuilderAtEnd(curr_builder, block);
      Value *body_val = body->gen_value();
      LLVMBuildRet(curr_builder,
                   body_val->cast_to(type->return_type)->gen_val());
      curr_func_type = prev_func_type;
      unnamed_acc = prev_unnamed;
      if (auto next = LLVMGetNextInstruction(call))
        LLVMPositionBuilderBefore(curr_builder, next);
      else
        LLVMPositionBuilderAtEnd(curr_builder, LLVMGetInstructionParent(call));
    }
    pop_scope();
    return new ConstValue(type->return_type, call);
  }

  FuncValue *gen_ptr() {
    if (ft.vararg ? args.size() < this->args.size()
                  : args.size() != this->args.size())
      error("wrong number of arguments to function "
            << name << " (expected " << this->args.size() << ", got "
            << args.size() << ")");
    push_scope();
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
      for (size_t i = 0; i < type->arguments.size(); i++)
        curr_scope->set_variable(
            this->args[i].first,
            new NamedValue(new ConstValue(type->arguments[i],
                                          LLVMGetParam(declaration->func, i)),
                           this->args[i].first));
      FunctionType *prev_func_type = curr_func_type;
      curr_func_type = type;
      size_t prev_unnamed = unnamed_acc;
      unnamed_acc = 0;
      LLVMBasicBlockRef block = LLVMAppendBasicBlock(declaration->func, "");
      LLVMPositionBuilderAtEnd(curr_builder, block);
      Value *body_val = body->gen_value();
      LLVMBuildRet(curr_builder,
                   body_val->cast_to(type->return_type)->gen_val());
      unnamed_acc = prev_unnamed;
      curr_func_type = prev_func_type;
      if (position_back_to) {
        if (auto next = LLVMGetNextInstruction(position_back_to))
          LLVMPositionBuilderBefore(curr_builder, next);
        else
          LLVMPositionBuilderAtEnd(curr_builder,
                                   LLVMGetInstructionParent(position_back_to));
        LLVMInstructionEraseFromParent(position_back_to);
      }
    }
    pop_scope();
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
            std::vector<std::pair<std::string, TypeAST *>> args, bool vararg,
            TypeAST *return_type = nullptr, ExprAST *body = nullptr)
      : FunctionAST(name, add_this_type(args, this_type), vararg, return_type,
                    body),
        this_type(this_type) {}
  std::string get_name(FunctionType *type) {
    return this_type->stringify() + "." + FunctionAST::get_name(type);
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