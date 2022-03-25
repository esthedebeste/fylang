#pragma once
#include "asts.cpp"
#include "lexer.cpp"
#include "types.cpp"
#include "utils.cpp"
static int curr_token;
static int get_next_token() { return curr_token = next_token(); }
static int eat(const int expected_token, const char *exp_name = nullptr) {
  if (curr_token != expected_token) {
    if (exp_name == nullptr)
      fprintf(stderr, "Error: Unexpected token '%c' (%d), expected '%c'",
              curr_token, curr_token, expected_token);
    else
      fprintf(stderr, "Error: Unexpected token '%c' (%d), expected '%s'",
              curr_token, curr_token, exp_name);
    exit(1);
  } else
    return get_next_token();
}
static ExprAST *parse_expr();
static ExprAST *parse_primary();
static ExprAST *parse_unary();

static Type *parse_type_unary();
static Type *parse_function_type() {
  eat(T_FUNCTION, (char *)"fun");
  eat('(');
  Type **arg_types = alloc_arr<Type *>(64);
  size_t args_len = 0;
  bool vararg = false;
  if (curr_token != ')') {
    while (1) {
      if (curr_token == T_VARARG) {
        eat(T_VARARG);
        vararg = true;
        break;
      }
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
  return new FunctionType(return_type, arg_types, args_len, vararg);
}

static Type *parse_num_type() {
  char *id = identifier_string;
  size_t id_len = identifier_string_length;
  eat(T_IDENTIFIER, (char *)"identifier");
#define strlen(str) (sizeof(str) - 1)
#define check_type(str, is_floating, is_signed)                                \
  if (id_len >= strlen(str) && streql(id, str, strlen(str)))                   \
    if (id_len > strlen(str))                                                  \
      return new NumType(id + strlen(str), id_len - strlen(str), is_floating,  \
                         is_signed);                                           \
    else                                                                       \
      return new NumType(32, is_floating, is_signed)
  check_type("uint", false, false);
  else check_type("int", false, true);
  else check_type("float", true, true);
  else if (streq_lit(id, id_len, "void")) return new VoidType();
  else if (curr_named_types[std::string(
               id, id_len)]) return curr_named_types[std::string(id, id_len)];
  else {
    fprintf(stderr, "Error: invalid type '%s'", id);
    exit(1);
  }
#undef strlen
#undef check_type
}

static TypeDefAST *parse_type_definition() {
  eat(T_TYPE, (char *)"type");
  char *name = identifier_string;
  size_t name_len = identifier_string_length;
  eat(T_IDENTIFIER, (char *)"identifier");
  eat('=');
  Type *t = parse_type_unary();
  return new TypeDefAST(name, name_len, t);
}

static Type *parse_type_unary() {
  if (curr_token == T_FUNCTION)
    return parse_function_type();
  if (curr_token == T_TYPEOF) {
    eat(T_TYPEOF);
    auto expr = parse_expr();
    return expr->get_type();
  }
  // If the current token is not a unary type operator, just parse type
  if (!(curr_token == '&' || curr_token == '*' || curr_token == T_UNSIGNED))
    return parse_num_type();
  // If this is a unary operator, read it.
  int opc = curr_token;
  get_next_token();
  if (Type *operand = parse_type_unary()) {
    if (opc == '*')
      return new PointerType(operand);
    else if (opc == '&') {
      if (PointerType *ptr = dynamic_cast<PointerType *>(operand))
        return ptr->get_points_to();
      else
        error("use of & type operator without pointer on right-hand side");
    } else if (opc == T_UNSIGNED) {
      if (NumType *num = dynamic_cast<NumType *>(operand))
        if (num->is_floating)
          error("unsigned floats don't exist");
        else
          return new NumType(num->bits, false, false);
      else
        error("use of `unsigned` type operator without number on right-hand "
              "side");
    } else if (opc == T_SIGNED) {
      if (NumType *num = dynamic_cast<NumType *>(operand))
        return new NumType(num->bits, num->is_floating, true);
      else
        error(
            "use of `signed` type operator without number on right-hand side");
    }
  }
  error("unknown unary op");
}
static Type *parse_type_postfix() {
  Type *prev = parse_type_unary();
  while (1) {
    switch (curr_token) {
    case '[': {
      eat('[');
      if (curr_token == ']') {
        eat(']');
        prev = new PointerType(prev);
      }
      char *num = num_value;
      size_t num_len = num_length;
      if (num_has_dot)
        error("List lengths have to be integers");
      eat(T_NUMBER);
      eat(']');
      prev = new TupleType(prev, parse_pos_int(num, num_len, 10));
    }
    default:
      return prev;
    }
  }
}
static ExprAST *parse_number_expr() {
  // TODO: parse number base (hex 0x, binary 0b, octal 0o)
  auto result =
      new NumberExprAST(num_value, num_length, num_type, num_has_dot, 10);
  get_next_token(); // consume the number
  return result;
}
static ExprAST *parse_char_expr() {
  auto result = new CharExprAST(char_value);
  get_next_token(); // consume char
  return result;
}
static ExprAST *parse_string_expr() {
  auto result = new StringExprAST(string_value, string_length);
  get_next_token(); // consume string
  return result;
}
/// parenexpr ::= '(' expression ')'
static ExprAST *parse_paren_expr() {
  eat('(');
  auto expr = parse_expr();
  eat(')');
  return expr;
}

/// identifierexpr
///   ::= identifier
static ExprAST *parse_identifier_expr() {
  char *id = identifier_string;
  size_t id_len = identifier_string_length;
  eat(T_IDENTIFIER, (char *)"identifier");
  return new VariableExprAST(id, id_len);
}
/// ifexpr ::= 'if' (expression) expression 'else' expression
static ExprAST *parse_if_expr() {
  eat(T_IF);

  auto cond = parse_paren_expr();
  auto then = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  return new IfExprAST(cond, then, elze);
}
/// whileexpr ::= 'while' (expression) expression else expression
static ExprAST *parse_while_expr() {
  eat(T_WHILE);
  auto cond = parse_paren_expr();
  auto then = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  return new WhileExprAST(cond, then, elze);
}
/// forexpr ::= 'for' (expr; expr; expr) expr else expr
static ExprAST *parse_for_expr() {
  eat(T_FOR);
  eat('(');
  auto init = parse_expr(); // let i = 0
  eat(';');                 // ;
  auto cond = parse_expr(); // i < 5
  eat(';');                 // ;
  auto post = parse_expr(); // i = i + 1
  eat(')');
  auto body = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  return new ForExprAST(init, cond, body, post, elze);
}
/// newexpr ::= 'new' type '{' (identifier '=' expr ',')* '}'
static ExprAST *parse_new_expr() {
  eat(T_NEW, (char *)"new");
  auto type = dynamic_cast<StructType *>(parse_num_type());
  if (!type)
    error("new with non-struct value");
  char **keys = alloc_arr<char *>(128);
  size_t *key_lens = alloc_arr<size_t>(128);
  ExprAST **values = alloc_arr<ExprAST *>(128);
  unsigned int key_count = 0;
  eat('{');
  if (curr_token != '}')
    while (1) {
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

static ExprAST *parse_block() {
  static const unsigned int MAX_EXPRS = 1024;
  eat('{');
  ExprAST **exprs = alloc_arr<ExprAST *>(MAX_EXPRS);
  unsigned int expr_i = 0;
  while (curr_token != '}')
    if (curr_token == T_EOF)
      error("unclosed block");
    else if (expr_i > MAX_EXPRS)
      error("too many exprs in block (>1024)");
    else if (curr_token == ';')
      eat(';'); // ignore ;
    else
      exprs[expr_i++] = parse_expr();
  exprs = (ExprAST **)realloc_arr<ExprAST *>(exprs, expr_i);
  eat('}');
  return new BlockExprAST(exprs, expr_i);
}

static LetExprAST *parse_let_expr(bool global = false) {
  bool constant = false;
  if (curr_token == T_CONST) {
    constant = true;
    eat(T_CONST);
  } else
    eat(T_LET, (char *)"let");
  char *id = identifier_string;
  size_t id_len = identifier_string_length;
  Type *type = nullptr;
  eat(T_IDENTIFIER, (char *)"variable name");
  // explicit typing
  if (curr_token == ':') {
    eat(':');
    type = parse_type_unary();
  }
  ExprAST *value = nullptr;
  // immediate assign
  if (curr_token == '=') {
    eat('=');
    value = parse_expr();
  }

  return new LetExprAST(id, id_len, type, value, constant, global);
}

static BoolExprAST *parse_bool_expr() {
  bool val = curr_token == T_TRUE;
  eat(curr_token);
  return new BoolExprAST(val);
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static ExprAST *parse_primary() {
  switch (curr_token) {
  default:
    fprintf(stderr,
            "Error: unknown token '%c' (%d) when expecting an expression",
            curr_token, curr_token);
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
  case T_FOR:
    return parse_for_expr();
  case T_LET:
  case T_CONST:
    return parse_let_expr();
  case T_NEW:
    return parse_new_expr();
  case T_TRUE:
  case T_FALSE:
    return parse_bool_expr();
  case '{':
    return parse_block();
  }
}
static int get_token_precedence() {
  if (binop_precedence.count(curr_token))
    return binop_precedence[curr_token];
  else
    return -1;
  int t_prec = binop_precedence[curr_token];
  if (t_prec <= 0)
    return -1;
  return t_prec;
}

struct ParseCallResult {
  ExprAST **args;
  size_t args_len = 0;
};
static ParseCallResult parse_call() {
  eat('(');
  ParseCallResult res;
  res.args = alloc_arr<ExprAST *>(64);
  res.args_len = 0;
  if (curr_token != ')') {
    while (1) {
      res.args[res.args_len++] = parse_expr();
      if (curr_token == ')')
        break;
      if (curr_token != ',')
        error("Expected ')' or ',' in argument list");
      get_next_token();
    }
  }
  eat(')');
  return res;
}
/// unary
///   ::= primary
///   ::= '.' identifier
static ExprAST *parse_postfix() {
  ExprAST *prev = parse_primary();
  while (1)
    switch (curr_token) {
    default:
      return prev;
    case '.': {
      eat('.');
      char *id = identifier_string;
      size_t id_len = identifier_string_length;
      eat(T_IDENTIFIER, (char *)"identifier");
      if (curr_token == '(') {
        ParseCallResult call = parse_call();
        prev =
            new MethodCallExprAST(id, id_len, prev, call.args, call.args_len);
      } else
        prev = new PropAccessExprAST(id, id_len, prev);
      break;
    }
    case '(': {
      // function call
      ParseCallResult call = parse_call();
      prev = new CallExprAST(prev, call.args, call.args_len);
      break;
    }
    case '[': {
      // indexing
      eat('[');
      ExprAST *index = parse_expr();
      eat(']');
      prev = new IndexExprAST(prev, index);
      break;
    }
    case T_AS: {
      // cast
      eat(T_AS);
      Type *cast_to = parse_type_unary();
      prev = new CastExprAST(prev, cast_to);
      break;
    }
    }
}
/// unary
///   ::= primary
///   ::= '!' unary
static ExprAST *parse_unary() {
  // If the current token is not an operator, it must be a primary expr.
  if (curr_token != T_RETURN && !isascii(curr_token) || curr_token == '(' ||
      curr_token == ',' || curr_token == '{')
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
static ExprAST *parse_bin_op_rhs(int expr_prec, ExprAST *LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    if (!binop_precedence.count(curr_token))
      return LHS; // doesn't exist
    int t_prec = binop_precedence[curr_token];
    bool op_assign = false;
    // If this is a binop that binds at least as tightly as the current
    // binop, consume it, otherwise we are done.
    if (t_prec < expr_prec)
      return LHS;

    // Okay, we know this is a binop.
    int bin_op = curr_token;
    get_next_token();        // eat binop
    if (curr_token == '=') { // do op and then assign to LHS
      op_assign = true;
      eat('=');
    }
    // Parse the primary expression after the binary operator.
    auto RHS = parse_unary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int next_prec = get_token_precedence();
    if (t_prec < next_prec)
      RHS = parse_bin_op_rhs(t_prec + 1, RHS);

    // Merge LHS/RHS.
    auto op = new BinaryExprAST(bin_op, LHS, RHS);
    if (op_assign) // assign LHS to op result
      LHS = new BinaryExprAST('=', LHS, op);
    else
      LHS = op;
  }
}
static ExprAST *parse_expr() {
  ExprAST *LHS = parse_unary();
  return parse_bin_op_rhs(0, LHS);
}
/// prototype
///   ::= id '(' id* ')'
///   ::= '(' type ')' id '(' id* ')'
static PrototypeAST *parse_prototype(Type *default_return_type = nullptr) {
  Type *this_t = nullptr;
  if (curr_token == '(') {
    // type method
    eat('(');
    this_t = parse_type_unary();
    eat(')');
  }
  char *fn_name = identifier_string;
  size_t fn_name_len = identifier_string_length;
  eat(T_IDENTIFIER, (char *)"identifier");
  eat('(');

  // Read the list of argument names.
  char **arg_names = alloc_arr<char *>(64);
  size_t *arg_name_lens = alloc_arr<size_t>(64);
  Type **arg_types = alloc_arr<Type *>(64);
  unsigned int arg_count = 0;
  bool vararg = false;

  if (curr_token != ')')
    while (1) {
      if (curr_token == T_VARARG) {
        eat(T_VARARG);
        vararg = true;
        break;
      }
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
  if (curr_token == ':') {
    eat(':');
    return_type = parse_type_unary();
  } else
    return_type = default_return_type;

  if (this_t)
    return new PrototypeAST(this_t, fn_name, fn_name_len, arg_names,
                            arg_name_lens, arg_types, arg_count, return_type,
                            vararg);
  else
    return new PrototypeAST(fn_name, fn_name_len, arg_names, arg_name_lens,
                            arg_types, arg_count, return_type, vararg);
}

/// definition ::= 'fun' prototype expression
static FunctionAST *parse_definition() {
  eat(T_FUNCTION, (char *)"fun");
  auto proto = parse_prototype(nullptr /* assume from body */);

  auto e = parse_expr();
  return new FunctionAST(proto, e);
}
/// declare
///   ::= 'fun' prototype
///   ::= 'let' identifier ':' type
static DeclareExprAST *parse_declare() {
  eat(T_DECLARE, (char *)"declare"); // eat declare.
  if (curr_token == T_FUNCTION) {
    eat(T_FUNCTION);
    PrototypeAST *proto = parse_prototype(new NumType(32, false, true));
    return new DeclareExprAST(proto);
  } else if (curr_token == T_CONST || curr_token == T_LET) {
    LetExprAST *let = parse_let_expr();
    return new DeclareExprAST(let);
  } else
    eat(0, (char *)"fun', 'let', or 'const");
  return nullptr;
}

/// struct
///   ::= 'struct' identifier '{' (identifier: type)* '}'
static StructAST *parse_struct() {
  eat(T_STRUCT, (char *)"struct"); // eat struct.
  char *struct_name = identifier_string;
  size_t struct_name_len = identifier_string_length;
  eat(T_IDENTIFIER, (char *)"identifier");
  eat('{');
  char **key_names = alloc_arr<char *>(128);
  Type **key_types = alloc_arr<Type *>(128);
  size_t *key_name_lens = alloc_arr<size_t>(128);
  unsigned int key_count = 0;
  if (curr_token != '}')
    while (1) {
      char *id = identifier_string;
      size_t id_len = identifier_string_length;
      eat(T_IDENTIFIER, (char *)"identifier");
      if (curr_token == ':') {
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
  key_name_lens = realloc_arr<size_t>(key_name_lens, key_count);
  key_types = realloc_arr<Type *>(key_types, key_count);
  return new StructAST(struct_name, struct_name_len, key_names, key_name_lens,
                       key_types, key_count);
}

/// include ::= 'include' string_expr
static char *parse_include() {
  eat(T_INCLUDE, (char *)"include");
  char *path = string_value;
  if (curr_token != T_STRING) {
    fprintf(
        stderr,
        "Error: Unexpected token after 'include': '%c' (%d), expected string",
        curr_token, curr_token);
    exit(1);
  }
  return path;
}