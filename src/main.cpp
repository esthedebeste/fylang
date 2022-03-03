#include "consts.cpp"
#include "asts.cpp"
#include "utils.cpp"
#include "types.cpp"
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <malloc.h>

FILE *current_file;
static int next_char()
{
    static char buf[1024 * 4];
    static char *p = buf;
    static int n = 0;
    if (n == 0)
    {
        n = fread(buf, 1, sizeof(buf), current_file);
        p = buf;
    }
    char ret = n-- > 0 ? *p++ : EOF;
    if (ret == EOF)
        fputs("[EOF]", stderr);
    else
        fputc(ret, stderr);
    return ret;
}
/// --- BEGIN LEXER --- ///
static unsigned int identifier_string_length;
static char *identifier_string;    // [a-zA-Z][a-zA-Z0-9]* - Filled in if T_IDENTIFIER
static char char_value;            // '[^']' - Filled in if T_CHAR
static char *num_value;            // Filled in if T_NUMBER
static unsigned int num_length;    // len(num_value) - Filled in if T_NUMBER
static bool num_has_dot;           // Whether num_value contains '.' - Filled in if T_NUMBER
static char num_type;              // Type of number. 'd' => double, 'f' => float, 'i' => int32, 'u' => uint32, 'b' => byte/char/uint8
static char *string_value;         // "[^"]*" - Filled in if T_STRING
static unsigned int string_length; // len(string_value) - Filled in if T_STRING

static int last_char = ' ';
static void read_str(bool (*predicate)(char), char **output, unsigned int *length)
{
    unsigned int curr_size = 512;
    char *str = alloc_c(curr_size);
    static unsigned int str_len = 0;
    str[0] = last_char;
    str_len = 1;
    while (predicate(last_char = next_char()))
    {
        if (str_len > curr_size)
        {
            curr_size *= 2;
            str = realloc_c(str, curr_size);
        }
        str[str_len] = last_char;
        str_len++;
    }
    str[str_len] = '\0';
    str = realloc_c(str, str_len + 1);
    *output = str;
    *length = str_len;
    return;
}
// isdigit(c) || c=='.'
static bool is_numish(char c)
{
    if (c == '.')
    {
        if (num_has_dot)
            error("number can't have multiple .s");
        num_has_dot = true;
        return true;
    }
    return isdigit(c);
}
// c != '"'
static bool isnt_quot(char c)
{
    return c != '"';
}
// (!isspace(c))
static bool isnt_space(char c)
{
    return !isspace(c);
}
static bool is_alphaish(char c)
{
    return isalpha(c) || isdigit(c) || c == '_';
}
static char get_escape(char escape_char)
{
    switch (escape_char)
    {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case '\'':
        return '\'';
    case '"':
        return '"';
    case '\\':
        return '\\';
    case '0':
        return '\0';
    default:
        fprintf(stderr, "Invalid escape '\\%c'", escape_char);
        return '\0';
    }
}

// Returns a token, or a number of the token's ASCII value.
static int next_token()
{
    if (last_char == EOF)
        return T_EOF;
    while (isspace(last_char))
    {
        last_char = next_char();
    }
    if (isalpha(last_char))
    {
        read_str(&is_alphaish, &identifier_string, &identifier_string_length);
        if (streq(identifier_string, identifier_string_length, "fun", 3))
            return T_FUNCTION;
        else if (streq(identifier_string, identifier_string_length, "extern", 6))
            return T_EXTERN;
        else if (streq(identifier_string, identifier_string_length, "if", 2))
            return T_IF;
        else if (streq(identifier_string, identifier_string_length, "else", 4))
            return T_ELSE;
        else if (streq(identifier_string, identifier_string_length, "let", 3))
            return T_LET;
        else if (streq(identifier_string, identifier_string_length, "const", 5))
            return T_CONST;
        else if (streq(identifier_string, identifier_string_length, "while", 5))
            return T_WHILE;
        return T_IDENTIFIER;
    }
    else if (isdigit(last_char) || last_char == '.')
    {
        // Number: [0-9]*.?[0-9]*
        num_has_dot = false;
        read_str(&is_numish, &num_value, &num_length);
        if (last_char == 'd' || last_char == 'l' || last_char == 'f' || last_char == 'i' || last_char == 'u' || last_char == 'b')
        {
            num_type = last_char;
            last_char = next_char();
        }
        else
        {
            // if floating-point, default to double (float64)
            if (num_has_dot)
                num_type = 'd';
            // if not floating-point, default to long (int64)
            else
                num_type = 'l';
        }
        return T_NUMBER;
    }
    else if (last_char == '"')
    {
        // String: "[^"]*"
        unsigned int curr_size = 512;
        char *str = alloc_c(curr_size);
        unsigned int str_len = 0;
        while ((last_char = next_char()) != '"')
        {
            if (last_char == EOF)
                error("Unexpected EOF in string");
            if (str_len > curr_size)
            {
                curr_size *= 2;
                str = realloc_c(str, curr_size);
            }
            if (last_char == '\\')
                str[str_len] = get_escape(next_char());
            else
                str[str_len] = last_char;
            str_len++;
        }
        str[str_len] = '\0';
        str_len++;
        str = realloc_c(str, str_len + 1);
        string_value = str;
        string_length = str_len;
        last_char = next_char();
        return T_STRING;
    }
    else if (last_char == '#')
    {
        // #[^\n\r]*
        do
            last_char = next_char();
        while (last_char != EOF && last_char != '\n' && last_char != '\r');
        return next_token(); // could recurse overflow, might make a wrapper function that while's and a T_COMMENT type
    }
    else if (last_char == '\'')
    {
        // Char: '[^']'
        last_char = next_char(); // eat '
        if (last_char == EOF || last_char == '\n' || last_char == '\r')
            error("Unterminated char\n");
        if (last_char == '\\')
            char_value = get_escape(next_char());
        else
            char_value = last_char;
        if (next_char() != '\'') // eat '
            error("char with length above 1");
        last_char = next_char();
        return T_CHAR;
    }

    int curr_char = last_char;
    last_char = next_char();
    if (curr_char == '=' && last_char == '=') // ==
    {
        last_char = next_char();
        return T_EQEQ;
    }
    // Otherwise, just return the character as its ascii value.
    return curr_char;
}
/// --- END LEXER --- ///
/// --- BEGIN PARSER --- ///
static int curr_token;
static int get_next_token()
{
    return curr_token = next_token();
}
static int eat(int expected_token, char *exp_name = nullptr)
{
    if (curr_token != expected_token)
    {
        char *exp;
        if (exp_name == nullptr)
        {
            exp = alloc_c(2);
            exp[0] = expected_token;
            exp[1] = '\0';
        }
        else
            exp = exp_name;
        fprintf(stderr, "Error: Unexpected token '%c' (%d), expected '%s'", curr_token, curr_token, exp);
        exit(1);
    }
    else
        return get_next_token();
}
static ExprAST *parse_expr();
static ExprAST *parse_primary();
static ExprAST *parse_unary();

static Type *parse_type();
static Type *parse_type_unary()
{
    // If the current token is not a unary type operator, just parse type
    if (!(curr_token == '&' || curr_token == '*'))
        return parse_type();

    // If this is a unary operator, read it.
    int opc = curr_token;
    get_next_token();
    if (Type *operand = parse_type_unary())
    {
        if (opc == '*')
            return new PointerType(operand);
        else if (opc == '&')
        {
            if (PointerType *ptr = dynamic_cast<PointerType *>(operand))
                return ptr->get_points_to();
            else
                error("use of & type operator without pointer on right-hand side");
        }
    }
    return nullptr;
}

static Type *parse_type()
{
    bool is_signed = true;
    while (1)
    {
        char *id = identifier_string;
        unsigned int id_len = identifier_string_length;
        eat(T_IDENTIFIER, (char *)"identifier");
        if (streq(id, id_len, "unsigned", 8))
            is_signed = false;
        // starts with "int"
        else if (id_len >= 3 && streq(id, 3, "int", 3))
            if (id_len > 3)
                return new NumType(id + 3, id_len - 3, false, is_signed);
            else
                return new NumType(32, false, is_signed);
        // starts with "float"
        else if (id_len >= 5 && streq(id, 5, "float", 5))
            if (!is_signed)
                error("unsigned floats don't exist");
            else if (id_len > 5)
                return new NumType(id + 5, id_len - 5, true, true);
            else
                return new NumType(32, true, true);
        else if (streq(id, id_len, "double", 6))
            return new NumType(64, true, true);
        else if (streq(id, id_len, "byte", 4) || streq(id, id_len, "char", 4))
            return new NumType(8, false, is_signed);
        else if (streq(id, id_len, "long", 4))
            return new NumType(64, false, is_signed);
        else if (streq(id, id_len, "bool", 4))
            return new NumType(1, false, false);
        else if (streq(id, id_len, "void", 4))
            return new VoidType();
    }
}

static ExprAST *parse_number_expr()
{
    // TODO: parse number base (hex 0x, binary 0b, octal 0o)
    auto result = new NumberExprAST(num_value, num_length, num_type, num_has_dot, 10);
    get_next_token(); // consume the number
    return result;
}
static ExprAST *parse_char_expr()
{
    auto result = new CharExprAST(char_value);
    get_next_token(); // consume char
    return result;
}
static ExprAST *parse_string_expr()
{
    auto result = new StringExprAST(string_value, string_length);
    get_next_token(); // consume string
    return result;
}
/// parenexpr ::= '(' expression ')'
static ExprAST *parse_paren_expr()
{
    eat('(');
    auto expr = parse_expr();
    eat(')');
    return expr;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static ExprAST *parse_identifier_expr()
{
    char *id = identifier_string;
    unsigned int id_len = identifier_string_length;
    eat(T_IDENTIFIER, (char *)"identifier");

    if (curr_token != '(') // variable ref
        return new VariableExprAST(id, id_len);
    // function call

    eat('('); // eat (
    std::vector<ExprAST *> args;
    if (curr_token != ')')
    {
        while (1)
        {
            if (auto arg = parse_expr())
                args.push_back(arg);
            else
                return nullptr;

            if (curr_token == ')')
                break;

            if (curr_token != ',')
                error("Expected ')' or ',' in argument list");
            get_next_token();
        }
    }

    // Eat the ')'.
    get_next_token();

    return new CallExprAST(id, id_len, args);
}
/// ifexpr ::= 'if' (expression) expression 'else' expression
static ExprAST *parse_if_expr()
{
    eat(T_IF, (char *)"if");

    auto cond = parse_paren_expr();
    auto then = parse_expr();
    // todo: if without else support
    eat(T_ELSE, (char *)"else");
    auto elze = parse_expr();

    return new IfExprAST(cond, then,
                         elze);
}
/// whileexpr ::= 'while' (expression) expression else expression
static ExprAST *parse_while_expr()
{
    eat(T_WHILE, (char *)"while");

    auto cond = parse_paren_expr();
    auto then = parse_expr();
    // todo: while without else support
    eat(T_ELSE, (char *)"else");
    auto elze = parse_expr();

    return new WhileExprAST(cond, then, elze);
}

static ExprAST *parse_block()
{
    static const unsigned int MAX_EXPRS = 1024;
    eat('{');
    ExprAST **exprs = alloc_arr<ExprAST *>(MAX_EXPRS);
    unsigned int expr_i = 0;
    for (; curr_token != '}'; expr_i++)
        if (curr_token == T_EOF)
            error("unclosed block");
        else if (expr_i > MAX_EXPRS)
            error("too many exprs in block (>1024)");
        else if (curr_token == ';')
            continue; // ignore ;
        else
            exprs[expr_i] = parse_expr();
    exprs = (ExprAST **)realloc_arr<ExprAST *>(exprs, expr_i);
    eat('}');
    return new BlockExprAST(exprs, expr_i);
}

static LetExprAST *parse_let_expr(bool global = false)
{
    bool constant = false;
    if (curr_token == T_CONST)
    {
        constant = true;
        get_next_token();
    }
    else
        eat(T_LET, (char *)"let");
    char *id = identifier_string;
    unsigned int id_len = identifier_string_length;
    Type *type = nullptr;
    eat(T_IDENTIFIER, (char *)"variable name");
    // explicit typing
    if (curr_token == ':')
    {
        eat(':');
        type = parse_type_unary();
    }
    ExprAST *value = nullptr;
    // immediate assign
    if (curr_token == '=')
    {
        eat('=');
        value = parse_expr();
    }

    return new LetExprAST(id, id_len, type, value, constant, global);
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static ExprAST *parse_primary()
{
    switch (curr_token)
    {
    default:
        fprintf(stderr, "Error: unknown token '%c' (%d) when expecting an expression", curr_token, curr_token);
        exit(1);
    case T_IDENTIFIER:
        return parse_identifier_expr();
    case T_NUMBER:
        return parse_number_expr();
    case T_CHAR:
        return parse_char_expr();
    case T_STRING:
        return parse_string_expr();
    case '(':
        return parse_paren_expr();
    case T_IF:
        return parse_if_expr();
    case T_WHILE:
        return parse_while_expr();
    case T_LET:
    case T_CONST:
        return parse_let_expr();
    case '{':
        return parse_block();
    }
}
static std::map<char, int> binop_precedence;
static int get_token_precedence()
{
    if (curr_token < 0)
        return -1; // isn't a single-character token
    int t_prec = binop_precedence[curr_token];
    if (t_prec <= 0)
        return -1;
    return t_prec;
}
/// unary
///   ::= primary
///   ::= '!' unary
static ExprAST *parse_unary()
{
    // If the current token is not an operator, it must be a primary expr.
    if (!isascii(curr_token) || curr_token == '(' || curr_token == ',' || curr_token == '{')
        return parse_primary();

    // If this is a unary operator, read it.
    int opc = curr_token;
    get_next_token();
    if (auto operand = parse_unary())
        return new UnaryExprAST(opc, operand);
    return nullptr;
}
/// binoprhs
///   ::= ('+' primary)*
static ExprAST *parse_bin_op_rhs(int expr_prec,
                                 ExprAST *LHS)
{
    // If this is a binop, find its precedence.
    while (true)
    {
        int t_prec = get_token_precedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (t_prec < expr_prec)
            return LHS;

        // Okay, we know this is a binop.
        int bin_op = curr_token;
        get_next_token(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = parse_unary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int next_prec = get_token_precedence();
        if (t_prec < next_prec)
        {
            RHS = parse_bin_op_rhs(t_prec + 1, RHS);
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS =
            new BinaryExprAST(bin_op, LHS, RHS);
    }
}
static ExprAST *parse_expr()
{
    ExprAST *LHS = parse_unary();
    return parse_bin_op_rhs(0, LHS);
}
/// prototype
///   ::= id '(' id* ')'
static PrototypeAST *parse_prototype(Type *default_return_type = nullptr)
{
    char *fn_name = identifier_string;
    unsigned int fn_name_len = identifier_string_length;
    eat(T_IDENTIFIER, (char *)"identifier");

    if (curr_token != '(')
    {
        fprintf(stderr, "Error: Unexpected token '%c' (%d), expected '%c'", curr_token, curr_token, '(');
        exit(1);
    }

    // Read the list of argument names.
    char **arg_names = alloc_arr<char *>(64);
    unsigned int *arg_name_lens = alloc_arr<unsigned int>(64);
    Type **arg_types = alloc_arr<Type *>(64);
    unsigned int arg_count = 0;

    get_next_token();
    if (curr_token != ')')
        while (1)
        {
            arg_names[arg_count] = identifier_string;
            arg_name_lens[arg_count] = identifier_string_length;
            eat(T_IDENTIFIER, (char *)"identifier");
            eat(':');
            arg_types[arg_count] = parse_type_unary();
            arg_count++;
            if (curr_token == ')')
                break;
            if (curr_token != ',')
                error("Expected ')' or ',' in argument list");
            get_next_token();
        }
    eat(')');
    Type *return_type;
    if (curr_token == ':')
    {
        eat(':');
        return_type = parse_type_unary();
    }
    else
        return_type = default_return_type;

    return new PrototypeAST(fn_name, fn_name_len, arg_names, arg_name_lens, arg_types, arg_count, return_type);
}

/// definition ::= 'fun' prototype expression
static FunctionAST *parse_definition()
{
    eat(T_FUNCTION, (char *)"fun");
    auto proto = parse_prototype(nullptr /* assume from body */);

    auto e = parse_expr();
    return new FunctionAST(proto, e);
}
/// external
///   ::= 'fun' prototype
///   ::= 'let' identifier ':' type
static ExternExprAST *parse_extern()
{
    eat(T_EXTERN, (char *)"extern"); // eat extern.
    if (curr_token == T_FUNCTION)
    {
        get_next_token();
        PrototypeAST *proto = parse_prototype(new NumType(32, false, true));
        return new ExternExprAST(proto);
    }
    else if (curr_token == T_CONST)
    {
        LetExprAST *let = parse_let_expr();
        return new ExternExprAST(let);
    }
    else
        eat(0, (char *)"fun' or 'const");
    return nullptr;
}
// /// toplevelexpr ::= expression
// static FunctionAST* parse_top_level_expr()
// {
//     if (auto E = parse_expr())
//     {
//         // Make an anonymous proto.
//         auto Proto = new PrototypeAST(nullptr, 0, nullptr, 0);
//         return new FunctionAST(Proto, E);
//     }
//     return nullptr;
// }
static void handle_definition()
{
    if (auto ast = parse_definition())
    {
        fprintf(stderr, "Parsed a function definition.\n");
        ast->print_codegen_to(stderr);
    }
    else
        // Skip token for error recovery.
        get_next_token();
}

static void handle_extern()
{
    if (auto ast = parse_extern())
    {
        fprintf(stderr, "Parsed an extern\n");
        ast->print_codegen_to(stderr);
    }
    else
        // Skip token for error recovery.
        get_next_token();
}
static void handle_global_let()
{
    if (auto ast = parse_let_expr(true))
    {
        fprintf(stderr, "Parsed a global variable\n");
        ast->print_codegen_to(stderr);
    }
    else
        // Skip token for error recovery.
        get_next_token();
}

/// top ::= definition | external | expression | ';'
static void main_loop()
{
    get_next_token();
    while (1)
    {
        switch (curr_token)
        {
        case T_EOF:
            return;
        case ';': // ignore top-level semicolons.
            get_next_token();
            break;
        case T_FUNCTION:
            handle_definition();
            break;
        case T_EXTERN:
            handle_extern();
            break;
        case T_CONST:
        case T_LET:
            handle_global_let();
            break;
        default:
            fprintf(stderr, "Unexpected token '%c' (%d) at top-level", curr_token, curr_token);
            exit(1);
            break;
        }
    }
}
/// --- END PARSER --- ///
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <filename> <output>\n", argv[0]);
        return 1;
    }

    binop_precedence['='] = 1; // lowest
    binop_precedence['<'] = 10;
    binop_precedence['>'] = 10;
    binop_precedence[T_EQEQ] = 10;
    binop_precedence['+'] = 20;
    binop_precedence['-'] = 20;
    binop_precedence['*'] = 40;
    binop_precedence['&'] = 60;
    binop_precedence['|'] = 60; // highest

    // host machine triple
    char *target_triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef target;
    char *error_message;
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    if (LLVMGetTargetFromTriple(target_triple, &target, &error_message) != 0)
        error(error_message);
    char *host_cpu_name = LLVMGetHostCPUName();
    char *host_cpu_features = LLVMGetHostCPUFeatures();
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(target, target_triple, host_cpu_name, host_cpu_features, LLVMCodeGenLevelAggressive, LLVMRelocStatic, LLVMCodeModelSmall);
    target_data = LLVMCreateTargetDataLayout(target_machine);
    // create module
    curr_module = LLVMModuleCreateWithName(argv[1]);
    // set target to current machine
    LLVMSetTarget(curr_module, target_triple);
    // create builder, context, and pass manager (for optimization)
    curr_builder = LLVMCreateBuilder();
    curr_ctx = LLVMGetGlobalContext();
    curr_pass_manager = LLVMCreateFunctionPassManagerForModule(curr_module);
    LLVMAddAnalysisPasses(target_machine, curr_pass_manager);
    LLVMInitializeFunctionPassManager(curr_pass_manager);

    // open .fy file
    current_file = fopen(argv[1], "r");
    if (!current_file)
    {
        fprintf(stderr, "File \'%s\' doesn't exist", argv[1]);
        return 1;
    }
    // parse and compile everything into LLVM IR
    main_loop();
    // export LLVM IR into other file
    char *output = LLVMPrintModuleToString(curr_module);
    FILE *output_file = fopen(argv[2], "w");
    fprintf(output_file, "%s", output);
    // dispose of a bunch of stuff
    LLVMDisposeMessage(output);
    LLVMFinalizeFunctionPassManager(curr_pass_manager);
    LLVMDisposeModule(curr_module);
    LLVMDisposeBuilder(curr_builder);
    LLVMDisposePassManager(curr_pass_manager);
    LLVMDisposeMessage(target_triple);
    LLVMDisposeMessage(host_cpu_name);
    LLVMDisposeMessage(host_cpu_features);
    LLVMDisposeTargetMachine(target_machine);
    return 0;
}