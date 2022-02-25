#include <vector>
#include <memory>
#include <map>
#include <string>
#include "utils.cpp"
#include "types.cpp"
static LLVMContextRef curr_ctx;
static LLVMBuilderRef curr_builder;
static LLVMModuleRef curr_module;
static LLVMPassManagerRef curr_pass_manager;
struct ValueWithType
{
    LLVMValueRef value;
    Type *type;
    ValueWithType(LLVMValueRef value, Type *type) : value(value), type(type) {}
};

static std::map<std::string, ValueWithType *> curr_named_values;
static std::map<std::string, FunctionType *> curr_functions;

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
    virtual ~ExprAST() {}
    virtual Type *get_type() = 0;
    virtual LLVMValueRef codegen() = 0;
    void print_codegen_to(FILE *stream)
    {
        LLVMValueRef val = this->codegen();
        char *str = LLVMPrintValueToString(val);
        fprintf(stream, "%s\n", str);
        LLVMDisposeMessage(str);
    }
};

NumType *num_char_to_type(char type_char, bool has_dot)
{
    switch (type_char)
    {
    case 'd':
        return new NumType(64, true, true);
    case 'f':
        return new NumType(32, true, true);
    case 'i':
        if (has_dot)
            error("'i' (int32) type can't have a '.'");
        return new NumType(32, false, true);
    case 'u':
        if (has_dot)
            error("'u' (uint32) type can't have a '.'");
        return new NumType(32, false, false);
    case 'l':
        if (has_dot)
            error("'l' (long, int64) type can't have a '.'");
        return new NumType(64, false, true);
    case 'b':
        if (has_dot)
            error("'b' (byte, uint8) type can't have a '.'");
        return new NumType(8, false, false);
    default:
        fprintf(stderr, "Error: Invalid number type id '%c'", type_char);
        exit(1);
        return nullptr;
    }
}
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
    char *val;
    unsigned int val_len;
    NumType *type;

public:
    NumberExprAST(char *val, unsigned int val_len, char type_char, bool has_dot) : val(val), val_len(val_len)
    {
        type = num_char_to_type(type_char, has_dot);
    }
    Type *get_type()
    {
        return type;
    }
    LLVMValueRef codegen()
    {
        if (type->is_floating)
            return LLVMConstRealOfStringAndSize(type->llvm_type(), val, val_len);
        else
            return LLVMConstIntOfStringAndSize(type->llvm_type(), val, val_len, 10);
    }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
    char *name;
    unsigned int name_len;

public:
    VariableExprAST(char *name, unsigned int name_len) : name(name), name_len(name_len) {}
    Type *get_type()
    {
        ValueWithType *V = curr_named_values[name];
        if (!V)
        {
            fprintf(stderr, "Variable '%s' doesn't exist.", name);
            exit(1);
        }
        return curr_named_values[name]->type;
    }
    LLVMValueRef codegen()
    {
        // Look this variable up in the function.
        ValueWithType *V = curr_named_values[name];
        if (!V)
            error("non-existent variable");
        return V->value;
    }
};

/// CharExprAST - Expression class for a single char ('a')
class CharExprAST : public ExprAST
{
    char charr;

public:
    CharExprAST(char charr) : charr(charr) {}
    Type *get_type()
    {
        return new NumType(8, false, false);
    }
    LLVMValueRef codegen()
    {
        return LLVMConstInt(int_8_type, charr, false);
    }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
    char op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    Type *get_type()
    {
        Type *lhs_t = LHS->get_type();
        Type *rhs_t = RHS->get_type();
        if (lhs_t->llvm_type() == rhs_t->llvm_type())
        {
            error("binexpr left-hand and right-hand side don't have the same type");
        }
        return lhs_t;
    }
    LLVMValueRef codegen()
    {
        LLVMValueRef L = LHS->codegen();
        LLVMValueRef R = RHS->codegen();
        if (!L || !R)
            return nullptr;

        // todo do something with type
        switch (op)
        {
        case '+':
            return LLVMBuildFAdd(curr_builder, L, R, "addtmp");
        case '-':
            return LLVMBuildFSub(curr_builder, L, R, "subtmp");
        case '*':
            return LLVMBuildFMul(curr_builder, L, R, "multmp");
        case '<':
            L = LLVMBuildFCmp(curr_builder, LLVMRealPredicate::LLVMRealULT, L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return LLVMBuildUIToFP(curr_builder, L, float_64_type,
                                   "booltmp");
        default:
            fprintf(stderr, "Error: invalid binary operator '%c'", op);
            exit(1);
        }
    }
};
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST
{
    char op;
    std::unique_ptr<ExprAST> operand;

public:
    UnaryExprAST(char op, std::unique_ptr<ExprAST> operand)
        : op(op), operand(std::move(operand)) {}
    Type *get_type()
    {
        // todo could be different when unary pointer operators are implemented later
        return operand->get_type();
    }
    LLVMValueRef codegen()
    {
        switch (op)
        {
        case '!':
            // shortcut for != 1
            return LLVMBuildFCmp(curr_builder, LLVMRealONE, operand->codegen(), LLVMConstReal(float_64_type, 1.0), "nottmp");
        case '-':
            // shortcut for 0-n
            return LLVMBuildFSub(curr_builder, LLVMConstReal(float_64_type, 0.0), operand->codegen(), "negtmp");
        default:
            fprintf(stderr, "Error: invalid unary operator '%c'", op);
            exit(1);
        }
    }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
    char *callee;
    unsigned int callee_len;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(char *callee, unsigned int callee_len, std::vector<std::unique_ptr<ExprAST>> args)
        : callee(callee), callee_len(callee_len), args(std::move(args)) {}

    Type *get_type()
    {
        FunctionType *func = curr_functions[std::string(callee)];
        if (!func)
        {
            fprintf(stderr, "Error: Function '%s' doesn't exist", callee);
            exit(1);
        }
        return func->return_type;
    }
    LLVMValueRef codegen()
    {
        // Look up the name in the global module table.

        LLVMValueRef callee_f = LLVMGetNamedFunction(curr_module, callee);
        if (!callee_f)
            error("Unknown function referenced");

        // If argument mismatch error.
        if (LLVMCountParams(callee_f) != args.size())
            error("Incorrect # arguments passed");

        LLVMValueRef *args_v = (LLVMValueRef *)calloc(args.size(), sizeof(LLVMValueRef *));
        unsigned int args_v_len = args.size();
        for (unsigned i = 0, e = args.size(); i != e; ++i)
        {
            args_v[i] = args[i]->codegen();
        }
        // TODO: update to LLVMBuildCall2 by getting function type somehow
        return LLVMBuildCall(curr_builder, callee_f, args_v, args_v_len, "calltmp");
    }
};

class BlockExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> *exprs;
    unsigned int exprs_len;

public:
    BlockExprAST(std::unique_ptr<ExprAST> *exprs, unsigned int exprs_len)
        : exprs(exprs), exprs_len(exprs_len)
    {
        if (exprs_len == 0)
            error("block can't be empty.");
    }
    Type *get_type()
    {
        return exprs[exprs_len - 1]->get_type();
    }
    LLVMValueRef codegen()
    {
        // generate code for all exprs and only return last expr
        for (unsigned int i = 0; i < exprs_len - 1; i++)
            exprs[i]->codegen();
        return exprs[exprs_len - 1]->codegen();
    }
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST
{

public:
    char **arg_names;
    unsigned int *arg_name_lengths;
    Type **arg_types;
    Type *return_type;
    FunctionType *type;
    unsigned int arg_count;
    char *name;
    unsigned int name_len;
    PrototypeAST(char *name, unsigned int name_len,
                 char **arg_names, unsigned int *arg_name_lengths,
                 Type **arg_types,
                 unsigned int args_len,
                 Type *return_type)
        : name(name), name_len(name_len), arg_names(arg_names), arg_name_lengths(arg_name_lengths), arg_types(arg_types), arg_count(args_len), return_type(return_type)
    {
        type = curr_functions[std::string(name)] = new FunctionType(return_type, arg_types, args_len);
    }
    FunctionType *get_type()
    {
        return type;
    }
    LLVMValueRef codegen()
    {
        // Make the function type:  double(double,double) etc.
        LLVMTypeRef *llvm_arg_types = (LLVMTypeRef *)calloc(arg_count, sizeof(LLVMTypeRef));
        for (unsigned i = 0; i != arg_count; ++i)
            llvm_arg_types[i] = arg_types[i]->llvm_type();
        LLVMTypeRef function_type =
            LLVMFunctionType(return_type->llvm_type(), llvm_arg_types, arg_count, false);

        LLVMValueRef func =
            LLVMAddFunction(curr_module, name, function_type);
        // Set names for all arguments.
        LLVMValueRef *params = (LLVMValueRef *)calloc(arg_count, sizeof(LLVMValueRef));
        LLVMGetParams(func, params);
        for (unsigned i = 0; i != arg_count; ++i)
            LLVMSetValueName2(params[i], arg_names[i], arg_name_lengths[i]);

        LLVMSetValueName2(func, name, name_len);
        return func;
    }
    void print_codegen_to(FILE *stream)
    {
        LLVMValueRef val = this->codegen();
        char *str = LLVMPrintValueToString(val);
        fprintf(stream, "%s\n", str);
        LLVMDisposeMessage(str);
    }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
        : proto(std::move(proto)), body(std::move(body))
    {
    }

    LLVMValueRef codegen()
    {
        // First, check for an existing function from a previous 'extern' declaration.
        LLVMValueRef func = LLVMGetNamedFunction(curr_module, proto->name);

        if (!func)
            func = proto->codegen();

        if (!func)
            return nullptr;

        if (LLVMCountBasicBlocks(func) != 0)
            error("Function cannot be redefined.");

        auto block = LLVMAppendBasicBlockInContext(curr_ctx, func, "");
        LLVMPositionBuilderAtEnd(curr_builder, block);
        curr_named_values.clear();

        unsigned int args_len = LLVMCountParams(func);
        LLVMValueRef *params = (LLVMValueRef *)calloc(args_len, sizeof(LLVMValueRef *));
        LLVMGetParams(func, params);
        size_t unused = 0;
        for (unsigned i = 0; i != args_len; ++i)
            curr_named_values[LLVMGetValueName2(params[i], &unused)] = new ValueWithType(params[i], proto->arg_types[i]);
        if (LLVMValueRef ret_val = body->codegen())
        {
            // Finish off the function.
            LLVMBuildRet(curr_builder, ret_val);

            // doesnt exist in c api (i think)
            // // Validate the generated code, checking for consistency.
            // // verifyFunction(*TheFunction);

            return func;
        }
        // Error reading body, remove function.
        LLVMDeleteFunction(func);
        return nullptr;
    }
    void print_codegen_to(FILE *stream)
    {
        LLVMValueRef val = this->codegen();
        char *str = LLVMPrintValueToString(val);
        fprintf(stream, "%s\n", str);
        LLVMDisposeMessage(str);
    }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> cond, then, elze;

public:
    IfExprAST(std::unique_ptr<ExprAST> cond, std::unique_ptr<ExprAST> then,
              // elze because else cant be a variable name lol
              std::unique_ptr<ExprAST> elze)
        : cond(std::move(cond)), then(std::move(then)), elze(std::move(elze))
    {
    }

    Type *get_type()
    {
        Type *then_t = then->get_type();
        Type *else_t = elze->get_type();
        if (!then_t->eq(else_t))
        {
            error("if then and else side don't have the same type");
        }
        return then_t;
    }

    LLVMValueRef codegen()
    {
        LLVMValueRef cond_v = cond->codegen();
        cond_v = LLVMBuildFCmp(curr_builder, LLVMRealONE, cond_v, LLVMConstReal(float_64_type, 0.0), "ifcond");
        LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(curr_builder));

        LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(curr_ctx, func, "then");
        LLVMBasicBlockRef else_bb = LLVMCreateBasicBlockInContext(curr_ctx, "else");
        LLVMBasicBlockRef merge_bb = LLVMCreateBasicBlockInContext(curr_ctx, "ifcont");
        // if
        LLVMBuildCondBr(curr_builder, cond_v, then_bb, else_bb);
        // then
        LLVMPositionBuilderAtEnd(curr_builder, then_bb);
        LLVMValueRef then_v = then->codegen();
        LLVMBuildBr(curr_builder, merge_bb);
        // Codegen of 'then' can change the current block, update then_bb for the PHI.
        then_bb = LLVMGetInsertBlock(curr_builder);
        // else
        LLVMAppendExistingBasicBlock(func, else_bb);
        LLVMPositionBuilderAtEnd(curr_builder, else_bb);
        LLVMValueRef else_v = elze->codegen();
        LLVMBuildBr(curr_builder, merge_bb);
        // Codegen of 'else' can change the current block, update else_bb for the PHI.
        else_bb = LLVMGetInsertBlock(curr_builder);
        // merge
        LLVMAppendExistingBasicBlock(func, merge_bb);
        LLVMPositionBuilderAtEnd(curr_builder, merge_bb);
        LLVMValueRef phi = LLVMBuildPhi(curr_builder, float_64_type, "iftmp");
        // todo merge idk
        LLVMAddIncoming(phi, &then_v, &then_bb, 1);
        LLVMAddIncoming(phi, &else_v, &else_bb, 1);
        return phi;
    }
};