#include "functions.h"
#include "asts.h"

ReturnState curr_return_state;
void add_return(LLVMValueRef ret_val, LLVMBasicBlockRef curr_block) {
  LLVMBuildBr(curr_builder, curr_return_state.return_block);
  LLVMAddIncoming(curr_return_state.return_phi, &ret_val, &curr_block, 1);
}
void add_return(Value *ret_val, LLVMBasicBlockRef curr_block) {
  return add_return(ret_val->cast_to(curr_return_state.return_type)->gen_val(),
                    curr_block);
}

class MethodAST;
class FunctionAST;
std::unordered_map<std::string, std::vector<MethodAST *>>
    curr_extension_methods;
std::vector<FunctionAST *> always_compile_functions;

FuncFlags flags;
FunctionAST::FunctionAST(std::string name,
                         std::vector<std::pair<std::string, TypeAST *>> args,
                         FuncFlags flags, TypeAST *return_type, ExprAST *body)
    : name(name), args(args), flags(flags), body(body),
      ft(return_type, seconds(args), flags), base_scope(curr_scope) {
  if (flags.always_compile)
    always_compile_functions.push_back(this);
}
std::string FunctionAST::get_name(FunctionType *type) {
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
FuncValue *FunctionAST::declare(FunctionType *type) {
  std::string name = curr_scope->get_prefix() + get_name(type);
  if (already_declared.count(type))
    return already_declared[type];
  if (auto func = LLVMGetNamedFunction(curr_module, name.c_str()))
    return already_declared[type] = new FuncValue(type, func);
  LLVMValueRef func =
      LLVMAddFunction(curr_module, name.c_str(), type->llvm_type());
  LLVMSetFunctionCallConv(func, flags.call_conv);
  return already_declared[type] = new FuncValue(type, func);
}
static FunctionType *get_func_type(FunctionAST *func) {
  for (auto &[name, type] : func->args)
    curr_scope->declare_variable(name, type->type());
  FunctionType *type = func->ft.func_type();
  if (!type->return_type && func->body)
    type->return_type = func->body->get_type();
  if (!type->return_type)
    error("can't infer return type of function " + func->name);
  return type;
}
FunctionType *FunctionAST::get_type() {
  auto prev_scope = curr_scope;
  curr_scope = new Scope(base_scope);
  auto type = get_func_type(this);
  curr_scope = prev_scope;
  return type;
}
FunctionType *FunctionAST::get_type(std::vector<Type *> args) {
  if (ft.flags.is_vararg ? args.size() < this->args.size()
                         : args.size() != this->args.size())
    error("wrong number of arguments to function "
          << name << " (expected " << this->args.size() << ", got "
          << args.size() << ")");
  auto prev_scope = curr_scope;
  curr_scope = new Scope(base_scope);
  for (size_t i = 0; i < args.size(); ++i) {
    if (i < this->args.size() && !this->args[i].second->castable_from(args[i]))
      error("argument " + std::to_string(i) + " to function " + name +
            " has wrong type, got: " + args[i]->stringify() +
            ", expected: " + this->args[i].second->stringify());
  }
  auto type = get_func_type(this);
  curr_scope = prev_scope;
  return type;
}
FunctionType *FunctionAST::get_type(std::vector<ExprAST *> args) {
  std::vector<Type *> arg_types;
  for (auto &arg : args)
    arg_types.push_back(arg->get_type());
  return get_type(arg_types);
}
// Returns PHI of return value, moves to return block
LLVMValueRef FunctionAST::gen_body(LLVMValueRef *args, FunctionType *type) {
  for (size_t i = 0; i < this->args.size(); i++) {
    curr_scope->set_variable(this->args[i].first,
                             new ConstValue(type->arguments[i], args[i]));
    LLVMSetValueName2(args[i], this->args[i].first.c_str(),
                      this->args[i].first.length());
  }
  ReturnState prev_return_state = curr_return_state;
  LLVMBasicBlockRef body_bb = LLVMGetInsertBlock(curr_builder);
  LLVMBasicBlockRef ret_bb = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(body_bb), ("return_" + name).c_str());
  LLVMPositionBuilderAtEnd(curr_builder, ret_bb);
  LLVMValueRef ret_phi =
      LLVMBuildPhi(curr_builder, type->return_type->llvm_type(), "retval");
  LLVMPositionBuilderAtEnd(curr_builder, body_bb);
  curr_return_state = {type->return_type, ret_bb, ret_phi};
  Value *body_val = body->gen_value();
  add_return(body_val);
  LLVMMoveBasicBlockAfter(ret_bb, LLVMGetInsertBlock(curr_builder));
  LLVMPositionBuilderAtEnd(curr_builder, ret_bb);
  curr_return_state = prev_return_state;
  return ret_phi;
}

ConstValue *FunctionAST::gen_call(std::vector<ExprAST *> args) {
  std::vector<Value *> arg_vals(args.size());
  for (size_t i = 0; i < args.size(); ++i)
    arg_vals[i] = args[i]->gen_value();
  return gen_call(arg_vals);
}
ConstValue *FunctionAST::gen_call(std::vector<Value *> arg_vals) {
  if (ft.flags.is_vararg ? arg_vals.size() < this->args.size()
                         : arg_vals.size() != this->args.size())
    error("wrong number of arguments to function "
          << name << " (expected " << this->args.size() << ", got "
          << arg_vals.size() << ")");
  auto prev_scope = curr_scope;
  curr_scope = new Scope(base_scope);
  for (size_t i = 0; i < arg_vals.size(); ++i) {
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
    debug_log("inlining function " << name);
    LLVMValueRef ret = gen_body(llvm_args, type);
    curr_scope = prev_scope;
    return new ConstValue(type->return_type, ret);
  }
  FuncValue *declaration = declare(type);
  LLVMValueRef call =
      LLVMBuildCall2(curr_builder, type->llvm_type(), declaration->func,
                     llvm_args, arg_vals.size(), ("call_" + name).c_str());
  LLVMSetInstructionCallConv(call, flags.call_conv);
  if (body && !LLVMGetFirstBasicBlock(declaration->func)) {
    debug_log("generating body for function " << name);
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

FuncValue *FunctionAST::gen_ptr() {
  auto prev_scope = curr_scope;
  curr_scope = new Scope(base_scope);
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
            ? LLVMBuildAlloca(curr_builder, NullType().llvm_type(), UN)
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

void FunctionAST::add() { curr_scope->set_function(name, this); }

auto add_this_type(std::vector<std::pair<std::string, TypeAST *>> args,
                   TypeAST *type) {
  args.insert(args.begin(), std::make_pair("this", type));
  return args;
}

MethodAST::MethodAST(TypeAST *this_type, std::string name,
                     std::vector<std::pair<std::string, TypeAST *>> args,
                     FuncFlags flags, TypeAST *return_type, ExprAST *body)
    : FunctionAST(name, add_this_type(args, this_type), flags, return_type,
                  body),
      this_type(this_type) {}
std::string MethodAST::get_name(FunctionType *type) {
  return this_type->type()->stringify() + "." + FunctionAST::get_name(type);
}
FunctionType *MethodAST::get_type(std::vector<ExprAST *> args,
                                  ExprAST *this_arg) {
  args.insert(args.begin(), this_arg);
  return FunctionAST::get_type(args);
}
FunctionType *MethodAST::get_type(std::vector<Type *> args, Type *this_arg) {
  args.insert(args.begin(), this_arg);
  return FunctionAST::get_type(args);
}
Value *MethodAST::gen_call(std::vector<ExprAST *> args, ExprAST *this_arg) {
  args.insert(args.begin(), this_arg);
  return FunctionAST::gen_call(args);
}
Value *MethodAST::gen_call(std::vector<Value *> args, Value *this_arg) {
  args.insert(args.begin(), this_arg);
  return FunctionAST::gen_call(args);
}
void MethodAST::add() { curr_extension_methods[name].push_back(this); }

#include "limits.h"
MethodAST *get_method(Type *this_type, std::string name) {
  if (!curr_extension_methods.count(name))
    return nullptr;
  uint min_generic = UINT_MAX;
  MethodAST *best_match = nullptr;
  for (auto method : curr_extension_methods[name]) {
    uint generic_count = 0;
    if (method->this_type->match(this_type, &generic_count) &&
        generic_count < min_generic) {
      min_generic = generic_count;
      best_match = method;
    }
  }
  return best_match;
}

MethodAST *Type::get_destructor() { return get_method(this, "__free__"); }