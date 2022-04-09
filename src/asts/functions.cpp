#pragma once
#include "asts.cpp"
#include "types.cpp"

class TopLevelAST {
public:
  virtual ~TopLevelAST() {}
  virtual LLVMValueRef gen_toplevel() = 0;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the
/// number of arguments the function takes).
class PrototypeAST {
public:
  std::string name;
  std::vector<std::pair<std::string, TypeAST *>> args;
  bool vararg;
  TypeAST *return_type;
  PrototypeAST(std::string name,
               std::vector<std::pair<std::string, TypeAST *>> args,
               TypeAST *return_type, bool vararg)
      : name(name), args(args), vararg(vararg), return_type(return_type) {}
  virtual void init_vars() {
    for (auto &[name, type] : args)
      curr_named_variables[name] = new ValueAndType(type->type());
  }
  ValueAndType *vt = nullptr;
  FunctionType *ft = nullptr;
  virtual FunctionType *get_type() {
    if (vt)
      return ft;
    std::vector<Type *> arg_types;
    for (auto &[name, type] : args)
      arg_types.push_back(type->type());
    ft = new FunctionType(return_type->type(), arg_types, vararg);
    vt = new ValueAndType(ft);
    return ft;
  }
  virtual LLVMValueRef codegen() {
    FunctionType *ft = get_type();
    LLVMValueRef func =
        LLVMAddFunction(curr_module, name.c_str(), ft->llvm_type());
    curr_func_type = ft;
    vt->value = new FuncValue(ft, func);
    curr_named_variables[name] = vt;
    // Set names for all arguments.
    LLVMValueRef *params = new LLVMValueRef[args.size()];
    LLVMGetParams(func, params);
    for (size_t i = 0; i < args.size(); ++i)
      LLVMSetValueName2(params[i], args[i].first.c_str(), args[i].first.size());

    LLVMSetValueName2(func, name.c_str(), name.size());
    LLVMPositionBuilderAtEnd(curr_builder, LLVMGetFirstBasicBlock(func));
    return func;
  }
  virtual void set_return_type(TypeAST *return_type) {
    this->return_type = return_type;
  }
};
class MethodAST : public PrototypeAST {
public:
  TypeAST *this_type;
  MethodAST(TypeAST *this_type, std::string name,
            std::vector<std::pair<std::string, TypeAST *>> args,
            TypeAST *return_type, bool vararg)
      : this_type(this_type), PrototypeAST(name, args, return_type, vararg) {}
  void init_vars() {
    PrototypeAST::init_vars();
    curr_named_variables[std::string("this", 4)] =
        new ValueAndType(this_type->type());
  }

  PrototypeAST *proto = nullptr;
  FunctionType *get_type() {
    if (proto)
      return proto->get_type();
    std::string compiled_name = this_type->stringify() + ":" + name;
    args.push_back(std::make_pair("this", this_type));
    proto = new PrototypeAST(compiled_name, args, return_type, vararg);
    FunctionType *type = proto->get_type();
    curr_extension_methods[{this_type->type(), name}] = proto->vt;
    return type;
  }

  LLVMValueRef codegen() {
    get_type();
    return proto->codegen();
  }

  void set_return_type(TypeAST *return_type) {
    this->return_type = return_type;
    if (proto)
      proto->return_type = return_type;
  }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public TopLevelAST {

public:
  PrototypeAST *proto;
  ExprAST *body;
  FunctionAST(PrototypeAST *proto, ExprAST *body) : proto(proto), body(body) {}

  LLVMValueRef gen_toplevel() {
    proto->init_vars();
    if (proto->return_type == nullptr)
      proto->set_return_type(type_ast(body->get_type()));
    // First, check for an existing function from a previous 'declare'
    // declaration.
    LLVMValueRef func = LLVMGetNamedFunction(curr_module, proto->name.c_str());

    if (!func)
      func = proto->codegen();

    if (!func)
      error("funcless behavior");

    if (LLVMCountBasicBlocks(func) != 0)
      error("Function cannot be redefined.");

    auto block = LLVMAppendBasicBlockInContext(curr_ctx, func, "");
    LLVMPositionBuilderAtEnd(curr_builder, block);
    size_t args_len = LLVMCountParams(func);
    LLVMValueRef *params = new LLVMValueRef[args_len];
    LLVMGetParams(func, params);
    for (unsigned i = 0; i != args_len; ++i) {
      size_t len = 0;
      const char *name = LLVMGetValueName2(params[i], &len);
      curr_named_variables[std::string(name, len)]->value =
          new ConstValue(proto->args[i].second->type(), params[i]);
    }
    Value *ret_val = body->gen_value()->cast_to(proto->get_type()->return_type);
    // Finish off the function.
    LLVMBuildRet(curr_builder, ret_val->gen_val());
    unnamed_acc = 0;
    return func;
    // doesnt exist in c api (i think)
    // // Validate the generated code, checking for consistency.
    // // verifyFunction(*TheFunction);
  }
};