#pragma once
#include "lexer.cpp"

#include "asts/asts.cpp"
#include "asts/functions.cpp"
#include "asts/types.cpp"
#include "types.cpp"
#include "utils.cpp"
static int curr_token;
static int get_next_token() { return curr_token = next_token(); }
static void eat(const int expected_token) {
  if (curr_token != expected_token)
    error("Unexpected token '" + token_to_str(curr_token) + "', expected '" +
          token_to_str(expected_token) + "'");
  get_next_token();
}
static ExprAST *parse_expr();
static ExprAST *parse_primary();
static ExprAST *parse_unary();
std::vector<ExprAST *> parse_call();

static TypeAST *parse_type_unary();
static TypeAST *parse_type_postfix();
inline TypeAST *parse_type() { return parse_type_unary(); }
static TypeAST *parse_function_type() {
  eat(T_FUNCTION);
  eat('(');
  std::vector<TypeAST *> arg_types;
  bool vararg = false;
  if (curr_token != ')') {
    while (1) {
      if (curr_token == T_VARARG) {
        eat(T_VARARG);
        vararg = true;
        break;
      }
      arg_types.push_back(parse_type());
      if (curr_token == ')')
        break;
      eat(',');
    }
  }
  eat(')');
  eat(':');
  TypeAST *return_type = parse_type();
  return new FunctionTypeAST(return_type, arg_types, vararg);
}

static TypeAST *parse_num_type() {
  std::string id = identifier_string;
  eat(T_IDENTIFIER);
#define strlen(str) (sizeof(str) - 1)
#define check_type(str, flt, sgn)                                              \
  if (id.find(str) == 0)                                                       \
    if (id.size() == strlen(str))                                              \
      return type_ast(new NumType(32, flt, sgn));                              \
    else                                                                       \
      return type_ast(new NumType(id.substr(strlen(str)), flt, sgn))
  check_type("uint", false, false);
  else check_type("int", false, true);
  else check_type("float", true, true);
  else if (curr_token == '<') {
    eat('<');
    std::vector<TypeAST *> args;
    while (true) {
      args.push_back(parse_type());
      if (curr_token == '>')
        break;
      eat(',');
    }
    eat('>');
    return new GenericAccessAST(id, args);
  }
  else return new NamedTypeAST(id);
#undef strlen
#undef check_type
}

static TypeDefAST *parse_type_definition() {
  eat(T_TYPE);
  std::string name = identifier_string;
  eat(T_IDENTIFIER);
  std::vector<std::string> generics;
  bool is_generic = false;
  if (curr_token == '<') {
    is_generic = true;
    eat('<');
    while (true) {
      generics.push_back(identifier_string);
      eat(T_IDENTIFIER);
      if (curr_token == '>')
        break;
      eat(',');
    }
    eat('>');
  }
  eat('=');
  TypeAST *t = parse_type();
  if (is_generic)
    return new GenericTypeDefAST(name, generics, t);
  else
    return new AbsoluteTypeDefAST(name, t);
}

static TypeAST *parse_tuple_type() {
  eat('{');
  std::vector<TypeAST *> types;
  if (curr_token != '}')
    while (1) {
      types.push_back(parse_type());
      if (curr_token == '}')
        break;
      eat(',');
    }
  eat('}');
  return new TupleTypeAST(types);
}
static TypeAST *parse_primary_type() {
  switch (curr_token) {
  case T_FUNCTION:
    return parse_function_type();
  case '{':
    return parse_tuple_type();
  case T_TYPEOF: {
    eat(T_TYPEOF);
    bool paren = curr_token == '(';
    if (paren)
      eat('(');
    TypeAST *type = new TypeofAST(parse_expr());
    if (paren)
      eat(')');
    return type;
  }
  case T_IDENTIFIER:
    return parse_num_type();
  }
  error("Unexpected token '" + token_to_str(curr_token) + "'");
}
static TypeAST *parse_type_postfix() {
  TypeAST *prev = parse_primary_type();
  while (1) {
    switch (curr_token) {
    case '[': {
      eat('[');
      if (curr_token == ']') {
        eat(']');
        prev = new UnaryTypeAST('*', prev);
      } else {
        std::string num = num_value;
        if (num_has_dot)
          error("List lengths have to be integers");
        eat(T_NUMBER);
        eat(']');
        prev = new ArrayTypeAST(prev, std::stoi(num));
      }
    }
    default:
      return prev;
    }
  }
}
static TypeAST *parse_type_unary() {
  if (curr_token == T_GENERIC) {
    eat(T_GENERIC);
    std::string name = identifier_string;
    eat(T_IDENTIFIER);
    return new GenericTypeAST(name);
  }
  // If the current token is not a unary type operator, just parse type
  if (type_unaries.count(curr_token) == 0)
    return parse_type_postfix();
  // if it is a unary type operator, parse it
  int opc = curr_token;
  eat(opc);
  TypeAST *operand = parse_type_unary();
  return new UnaryTypeAST(opc, operand);
}
static ExprAST *parse_number_expr() {
  // TODO: parse number base (hex 0x, binary 0b, octal 0o)
  auto result = new NumberExprAST(num_value, num_type, num_has_dot, 10);
  eat(T_NUMBER);
  return result;
}
static ExprAST *parse_char_expr() {
  auto result = new CharExprAST(char_value);
  eat(T_CHAR);
  return result;
}
static ExprAST *parse_string_expr() {
  auto result = new StringExprAST(string_value);
  eat(T_STRING);
  return result;
}
/// parenexpr ::= '(' expression ')'
static ExprAST *parse_paren_expr() {
  eat('(');
  auto expr = parse_expr();
  if (curr_token == ',') {
    eat(',');
    std::vector<ExprAST *> exprs;
    exprs.push_back(expr);
    if (curr_token != ')')
      while (1) {
        exprs.push_back(parse_expr());
        if (curr_token == ')')
          break;
        eat(',');
      }
    eat(')');
    return new TupleExprAST(exprs);
  }
  eat(')');
  return expr;
}

/// identifierexpr
///   ::= identifier
static ExprAST *parse_identifier_expr() {
  std::string id = identifier_string;
  eat(T_IDENTIFIER);
  if (curr_token == '(') {
    std::vector<ExprAST *> args = parse_call();
    return new NameCallExprAST(id, args);
  }
  return new VariableExprAST(id);
}
/// ifexpr
///   ::= 'if' (expression) expression ('else' expression)?
///   ::= 'if' ('type' type == type) expression ('else' expression)?
static ExprAST *parse_if_expr() {
  eat(T_IF);
  eat('(');
  enum { EXPR_IF, TYPE_IF } returning = EXPR_IF;
  ExprAST *cond;
  TypeAST *a, *b;
  if (curr_token == T_TYPE || curr_token == T_TYPEOF) {
    // Type condition
    if (curr_token == T_TYPE)
      eat(T_TYPE);
    a = parse_type();
    eat(T_EQEQ);
    b = parse_type();
    returning = TYPE_IF;
  } else
    cond = parse_expr();
  eat(')');
  auto then = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  switch (returning) {
  case EXPR_IF:
    return new IfExprAST(cond, then, elze);
  case TYPE_IF:
    return new TypeIfExprAST(a, b, then, elze);
  }
  return nullptr;
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
/// newexpr ::= 'new|create' type '{' (identifier '=' expr ',')* '}'
static ExprAST *parse_new_expr() {
  bool is_new = curr_token == T_NEW;
  eat(is_new ? T_NEW : T_CREATE);

  if (is_new && curr_token == '(') {
    // tuple on heap
    TupleExprAST *tuple = dynamic_cast<TupleExprAST *>(parse_paren_expr());
    if (tuple == nullptr)
      error("new tuple not a tuple, add a comma to the end");
    tuple->is_new = true;
    return tuple;
  }
  auto type = parse_primary_type();
  std::vector<std::pair<std::string, ExprAST *>> fields;
  eat('{');
  if (curr_token != '}')
    while (1) {
      std::string key = identifier_string;
      eat(T_IDENTIFIER);
      eat('=');
      auto value = parse_expr();
      fields.push_back(std::make_pair(key, value));
      if (curr_token == '}')
        break;
      eat(',');
    }
  eat('}');

  return new NewExprAST(type, fields, is_new);
}

static ExprAST *parse_block() {
  eat('{');
  std::vector<ExprAST *> exprs;
  while (curr_token != '}')
    if (curr_token == T_EOF)
      error("unclosed block");
    else if (curr_token == ';')
      eat(';'); // ignore ;
    else
      exprs.push_back(parse_expr());
  eat('}');
  return new BlockExprAST(exprs);
}

static LetExprAST *parse_let_expr() {
  bool constant = false;
  if (curr_token == T_CONST) {
    constant = true;
    eat(T_CONST);
  } else
    eat(T_LET);
  std::string id = identifier_string;
  TypeAST *type = nullptr;
  eat(T_IDENTIFIER);
  // explicit typing
  if (curr_token == ':') {
    eat(':');
    type = parse_type();
  }
  ExprAST *value = nullptr;
  // immediate assign
  if (curr_token == '=') {
    eat('=');
    value = parse_expr();
  }

  return new LetExprAST(id, type, value, constant);
}

static SizeofExprAST *parse_sizeof_expr() {
  eat(T_SIZEOF);
  bool paren = curr_token == '(';
  if (paren)
    eat('(');
  TypeAST *type = parse_type();
  if (paren)
    eat(')');
  return new SizeofExprAST(type);
}

static BoolExprAST *parse_bool_expr() {
  bool val = curr_token == T_TRUE;
  eat(curr_token);
  return new BoolExprAST(val);
}

static ExprAST *parse_type_assertion() {
  eat(T_ASSERT_TYPE);
  TypeAST *a = parse_type();
  eat(T_EQEQ);
  TypeAST *b = parse_type();
  return new TypeAssertExprAST(a, b);
}

static ExprAST *parse_type_dump() {
  eat(T_DUMP);
  TypeAST *a = parse_type();
  return new TypeDumpExprAST(a);
}
/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static ExprAST *parse_primary() {
  switch (curr_token) {
  default:
    error("Error: unexpected token '" + token_to_str(curr_token) +
          "' when expecting a primary");
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
  case T_CREATE:
  case T_NEW:
    return parse_new_expr();
  case T_TRUE:
  case T_FALSE:
    return parse_bool_expr();
  case T_SIZEOF:
    return parse_sizeof_expr();
  case T_DUMP:
    return parse_type_dump();
  case T_ASSERT_TYPE:
    return parse_type_assertion();
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

std::vector<ExprAST *> parse_call() {
  eat('(');
  std::vector<ExprAST *> args;
  if (curr_token != ')') {
    while (1) {
      args.push_back(parse_expr());
      if (curr_token == ')')
        break;
      eat(',');
    }
  }
  eat(')');
  return args;
}
static ExprAST *parse_postfix() {
  ExprAST *prev = parse_primary();
  while (1)
    switch (curr_token) {
    default:
      return prev;
    case '.': {
      eat('.');
      if (curr_token == T_NUMBER) {
        unsigned int idx = std::stoi(num_value);
        eat(T_NUMBER);
        prev = new NumAccessExprAST(idx, prev);
      } else {
        std::string id = identifier_string;
        eat(T_IDENTIFIER);
        if (curr_token == '(') {
          auto call = parse_call();
          prev = new MethodCallExprAST(id, prev, call);
        } else
          prev = new PropAccessExprAST(id, prev);
      }
      break;
    }
    case '(': {
      // function call
      auto call = parse_call();
      prev = new ValueCallExprAST(prev, call);
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
      TypeAST *cast_to = parse_type();
      prev = new CastExprAST(prev, cast_to);
      break;
    }
    }
}
/// unary
///   ::= primary
///   ::= '!' unary
static ExprAST *parse_unary() {
  if (unaries.count(curr_token) == 0) // not a unary op
    return parse_postfix();

  int opc = curr_token;
  eat(opc);
  ExprAST *operand = parse_unary();
  return new UnaryExprAST(opc, operand);
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
    eat(bin_op);
    if (curr_token == '=') { // do op and then assign to LHS
      op_assign = true;
      eat('=');
    }
    // Parse the primary expression after the binary operator.
    auto RHS = parse_unary();

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
static FunctionAST *parse_prototype(TypeAST *default_return_type = nullptr) {
  TypeAST *this_t = nullptr;
  if (curr_token == '(') {
    // type method
    eat('(');
    this_t = parse_type();
    eat(')');
  }
  std::string fn_name = identifier_string;
  eat(T_IDENTIFIER);
  eat('(');

  // Read the list of argument names.
  std::vector<std::pair<std::string, TypeAST *>> args;
  bool vararg = false;

  if (curr_token != ')')
    while (1) {
      if (curr_token == T_VARARG) {
        eat(T_VARARG);
        vararg = true;
        break;
      }
      std::string arg_name = identifier_string;
      eat(T_IDENTIFIER);
      eat(':');
      TypeAST *arg_type = parse_type();
      args.push_back(std::make_pair(arg_name, arg_type));
      if (curr_token == ')')
        break;
      eat(',');
    }
  eat(')');
  TypeAST *return_type;
  if (curr_token == ':') {
    eat(':');
    return_type = parse_type();
  } else
    return_type = default_return_type;

  if (this_t)
    return new MethodAST(this_t, fn_name, args, vararg, return_type);
  else
    return new FunctionAST(fn_name, args, vararg, return_type);
}

/// definition ::= 'fun' prototype expression
static FunctionAST *parse_definition() {
  eat(T_FUNCTION);
  FunctionAST *func = parse_prototype(nullptr /* assume from body */);
  auto body = parse_expr();
  func->body = body;
  return func;
}
/// declare
///   ::= 'fun' prototype
///   ::= 'let' identifier ':' type
static DeclareExprAST *parse_declare() {
  eat(T_DECLARE); // eat declare.
  if (curr_token == T_FUNCTION) {
    eat(T_FUNCTION);
    FunctionAST *func = parse_prototype(type_ast(new NumType(32, false, true)));
    return new DeclareExprAST(func);
  } else if (curr_token == T_CONST || curr_token == T_LET) {
    LetExprAST *let = parse_let_expr();
    return new DeclareExprAST(let);
  } else
    error("Unexpected 'declare " + token_to_str(curr_token) +
          "', expected 'fun', 'let', or 'const'");
}

/// struct
///   ::= 'struct' identifier '{' (identifier: type)* '}'
static TypeDefAST *parse_struct() {
  eat(T_STRUCT); // eat struct.
  std::string struct_name = identifier_string;
  eat(T_IDENTIFIER);
  std::vector<std::string> generic_params;
  bool is_generic = false;
  if (curr_token == '<') {
    is_generic = true;
    eat('<');
    while (true) {
      generic_params.push_back(identifier_string);
      eat(T_IDENTIFIER);
      if (curr_token == '>')
        break;
      eat(',');
    }
    eat('>');
  }
  eat('{');
  std::vector<std::pair<std::string, TypeAST *>> members;
  if (curr_token != '}')
    while (1) {
      std::string member_name = identifier_string;
      eat(T_IDENTIFIER);
      eat(':');
      TypeAST *member_type = parse_type();
      members.push_back(std::make_pair(member_name, member_type));
      if (curr_token == '}')
        break;
      eat(',');
    }
  eat('}');
  if (is_generic)
    return new GenericTypeDefAST(struct_name, generic_params,
                                 new StructTypeAST(struct_name, members));
  else
    return new AbsoluteTypeDefAST(struct_name,
                                  new StructTypeAST(struct_name, members));
}

/// include ::= 'include' string_expr
static std::string parse_include() {
  eat(T_INCLUDE);
  std::string path = string_value;
  if (curr_token != T_STRING) {
    error("Error: Unexpected token after 'include': '" +
          token_to_str(curr_token) + "', expected string");
  }
  return path;
}