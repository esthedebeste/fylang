#pragma once
#include "lexer.cpp"
#include "types.cpp"
#include "asts.cpp"
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

static Type *parse_type_unary();
static Type *parse_function_type()
{
    eat(T_FUNCTION, (char *)"fun");
    eat('(');
    Type **arg_types = alloc_arr<Type *>(64);
    unsigned int args_len = 0;
    if (curr_token != ')')
    {
        while (1)
        {
            if (auto arg = parse_type_unary())
                arg_types[args_len++] = arg;
            if (curr_token == ')')
                break;
            if (curr_token != ',')
                error("Expected ')' or ',' in argument list");
            get_next_token();
        }
    }
    eat(')');
    eat(':');
    Type *return_type = parse_type_unary();
    return new FunctionType(return_type, arg_types, args_len);
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
        else if (curr_named_structs[std::string(id, id_len)])
            return curr_named_structs[std::string(id, id_len)];
    }
}

static Type *parse_type_unary()
{
    if (curr_token == T_FUNCTION)
        return parse_function_type();
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
static ExprAST *parse_identifier_expr()
{
    char *id = identifier_string;
    unsigned int id_len = identifier_string_length;
    eat(T_IDENTIFIER, (char *)"identifier");
    return new VariableExprAST(id, id_len);
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
/// newexpr ::= 'new' type '{' (identifier '=' expr ',')* '}'
static ExprAST *parse_new_expr()
{
    eat(T_NEW, (char *)"new");
    auto type = dynamic_cast<StructType *>(parse_type());
    if (!type)
        error("new with non-struct value");
    char **keys = alloc_arr<char *>(128);
    unsigned int *key_lens = alloc_arr<unsigned int>(128);
    ExprAST **values = alloc_arr<ExprAST *>(128);
    unsigned int key_count = 0;
    eat('{');
    if (curr_token != '}')
        while (1)
        {
            keys[key_count] = identifier_string;
            key_lens[key_count] = identifier_string_length;
            eat(T_IDENTIFIER, (char *)"identifier");
            eat('=');
            values[key_count] = parse_expr();
            key_count++;
            if (curr_token == '}')
                break;
            eat(',');
        }
    eat('}');

    return new NewExprAST(type, keys, key_lens, values, key_count);
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
    case T_NEW:
        return parse_new_expr();
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
///   ::= '.' identifier
static ExprAST *parse_postfix()
{
    ExprAST *prev = parse_primary();
    while (1)
        switch (curr_token)
        {
        default:
            return prev;
        case '.':
        {
            eat('.');
            char *id = identifier_string;
            unsigned int id_len = identifier_string_length;
            eat(T_IDENTIFIER, (char *)"identifier");
            prev = new PropAccessExprAST(id, id_len, prev);
            break;
        }
        case '(':
        {
            // function call
            eat('(');
            ExprAST **args = alloc_arr<ExprAST *>(64);
            unsigned int args_len = 0;
            if (curr_token != ')')
            {
                while (1)
                {
                    if (auto arg = parse_expr())
                        args[args_len++] = arg;
                    else
                        return nullptr;
                    if (curr_token == ')')
                        break;
                    if (curr_token != ',')
                        error("Expected ')' or ',' in argument list");
                    get_next_token();
                }
            }
            eat(')');
            prev = new CallExprAST(prev, args, args_len);
            break;
        }
        }
}
/// unary
///   ::= primary
///   ::= '!' unary
static ExprAST *parse_unary()
{
    // If the current token is not an operator, it must be a primary expr.
    if (!isascii(curr_token) || curr_token == '(' || curr_token == ',' || curr_token == '{')
        return parse_postfix();

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

/// struct
///   ::= 'struct' identifier '{' (identifier: type)* '}'
static StructAST *parse_struct()
{
    eat(T_STRUCT, (char *)"struct"); // eat struct.
    char *struct_name = identifier_string;
    unsigned int struct_name_len = identifier_string_length;
    eat(T_IDENTIFIER, (char *)"identifier");
    eat('{');
    char **key_names = alloc_arr<char *>(128);
    Type **key_types = alloc_arr<Type *>(128);
    unsigned int *key_name_lens = alloc_arr<unsigned int>(128);
    unsigned int key_count = 0;
    if (curr_token != '}')
        while (1)
        {
            char *id = identifier_string;
            unsigned int id_len = identifier_string_length;
            eat(T_IDENTIFIER, (char *)"identifier");
            if (curr_token == ':')
            {
                key_names[key_count] = id;
                key_name_lens[key_count] = id_len;
                // value
                eat(':');
                key_types[key_count] = parse_type_unary();
                key_count++;
                if (curr_token == '}')
                    break;
                eat(',');
            }
        }
    eat('}');
    key_names = realloc_arr<char *>(key_names, key_count);
    key_name_lens = realloc_arr<unsigned int>(key_name_lens, key_count);
    key_types = realloc_arr<Type *>(key_types, key_count);
    return new StructAST(struct_name, struct_name_len, key_names, key_name_lens, key_types, key_count);
}