#include "../asts.h"

FunctionType *ValueCallExprAST::get_func_type() {
  FunctionType *func_t = dynamic_cast<FunctionType *>(called->get_type());
  if (!func_t) {
    if (PointerType *ptr = dynamic_cast<PointerType *>(called->get_type()))
      func_t = dynamic_cast<FunctionType *>(ptr->get_points_to());
    else
      error("Function doesn't exist or is not a function");
  }
  return func_t;
}

ValueCallExprAST::ValueCallExprAST(ExprAST *called, std::vector<ExprAST *> args)
    : called(called), args(args) {}

Type *ValueCallExprAST::get_type() {
  FunctionType *func_t = get_func_type();
  if (func_t->flags.is_vararg ? args.size() < func_t->arguments.size()
                              : args.size() != func_t->arguments.size())
    error("Incorrect # arguments passed. (Expected " +
          std::to_string(func_t->arguments.size()) + ", got " +
          std::to_string(args.size()) + ")");

  return func_t->return_type;
}
Value *ValueCallExprAST::gen_value() {
  FunctionType *func_t = get_func_type();
  Value *func_v = called->gen_value();
  if (!func_v)
    error("Unknown function referenced");
  LLVMValueRef func = func_v->gen_val();
  LLVMValueRef *arg_vs = new LLVMValueRef[args.size()];
  for (size_t i = 0; i < args.size(); i++) {
    if (i < func_t->arguments.size())
      arg_vs[i] =
          args[i]->gen_value()->cast_to(func_t->arguments[i])->gen_val();
    else
      arg_vs[i] = args[i]->gen_value()->gen_val();
  }
  auto call = LLVMBuildCall2(curr_builder, func_t->llvm_type(), func, arg_vs,
                             args.size(), UN);
  LLVMSetInstructionCallConv(call, func_t->flags.call_conv);
  return new ConstValue(func_t->return_type, call);
}

NameCallExprAST::NameCallExprAST(Identifier name, std::vector<ExprAST *> args)
    : name(name), args(args) {}
Type *NameCallExprAST::get_type() {
  if (auto function = get_function(name))
    return function->get_type(args)->return_type;
  else
    return ValueCallExprAST(new VariableExprAST(name), args).get_type();
}
Value *NameCallExprAST::gen_value() {
  if (auto function = get_function(name))
    return function->gen_call(args);
  else
    return ValueCallExprAST(new VariableExprAST(name), args).gen_value();
}

ExtAndPtr MethodCallExprAST::get_extension() {
  auto extension = get_method(source->get_type(), name);
  if (!extension)
    return {get_method(source->get_type()->ptr(), name), true};
  else
    return {extension, false};
  return {nullptr, false};
}

MethodCallExprAST::MethodCallExprAST(std::string name, ExprAST *source,
                                     std::vector<ExprAST *> args)
    : name(name), source(source), args(args) {}

Type *MethodCallExprAST::get_type() {
  auto ext = get_extension();
  if (ext.extension != nullptr)
    return ext.extension
        ->get_type(args, ext.is_ptr ? new UnaryExprAST('&', source) : source)
        ->return_type;
  else
    return ValueCallExprAST(new PropAccessExprAST(name, source), args)
        .get_type();
}
Value *MethodCallExprAST::gen_value() {
  auto ext = get_extension();
  if (ext.extension != nullptr)
    return ext.extension->gen_call(
        args, ext.is_ptr ? new UnaryExprAST('&', source) : source);
  else
    return ValueCallExprAST(new PropAccessExprAST(name, source), args)
        .gen_value();
}