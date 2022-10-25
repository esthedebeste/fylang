#include "parser.h"

int curr_token;
int get_next_token() { return curr_token = next_token(); }
void eat(const int expected_token) {
  if (curr_token != expected_token)
    error("Unexpected token '" + token_to_str(curr_token) + "', expected '" +
          token_to_str(expected_token) + "'");
  get_next_token();
}
std::string eat_string() {
  std::string ret = string_value;
  eat(T_STRING);
  while (curr_token == T_STRING) {
    ret += string_value; // concatenate strings
    eat(T_STRING);
  }
  return ret;
}
Identifier parse_identifier(std::string first = "") {
  std::vector<std::string> spaces;
  if (first == "") {
    spaces.push_back(identifier_string);
    eat(T_IDENTIFIER);
  } else
    spaces.push_back(first);

  while (curr_token == T_DOUBLE_COLON) {
    eat(T_DOUBLE_COLON);
    spaces.push_back(identifier_string);
    eat(T_IDENTIFIER);
  }
  std::string last = spaces.back();
  spaces.pop_back();
  return Identifier(spaces, last);
}

std::tuple<std::string, TypeAST *, FuncFlags>
parse_prototype_begin(bool parse_name, bool parse_this);
TypeAST *parse_function_type() {
  auto flags = std::get<FuncFlags>(parse_prototype_begin(false, false));
  eat('(');
  std::vector<TypeAST *> arg_types;
  while (curr_token != ')') {
    if (curr_token == T_VARARG) {
      eat(T_VARARG);
      flags.is_vararg = true;
      break;
    }
    auto type = parse_type();
    if (curr_token == ':' && dynamic_cast<NamedTypeAST *>(type)) {
      eat(':');
      // allow type: fun(a: int, b: int) by ignoring what's before the :
      type = parse_type();
    }
    arg_types.push_back(type);
    if (curr_token == ')')
      break;
    eat(',');
  }
  eat(')');
  eat(':');
  TypeAST *return_type = parse_type();
  return new FunctionTypeAST(return_type, arg_types, flags);
}

bool is_num(std::string str) {
  for (char c : str)
    if (c < '0' || c > '9')
      return false;
  return true;
}

NumType *numtype(std::string prefix, std::string value, bool is_float,
                 bool is_signed) {
  if (!value.starts_with(prefix))
    return nullptr;
  std::string post = value.substr(prefix.size());
  if (post.size() == 0)
    return new NumType(32, is_float, is_signed);
  if (is_num(post))
    return new NumType(std::stoi(post), is_float, is_signed);
  if (post == "_ptrsize")
    return new NumType(is_signed);
  return nullptr;
}

TypeAST *parse_num_type() {
  std::string id = identifier_string;
  eat(T_IDENTIFIER);
  if (id == "null")
    return type_ast(new NullType());
  else if (NumType *num = numtype("uint", id, false, false))
    return type_ast(num);
  else if (NumType *num = numtype("int", id, false, true))
    return type_ast(num);
  else if (NumType *num = numtype("float", id, true, true))
    return type_ast(num);
  else if (curr_token == '<') {
    eat('<');
    std::vector<TypeAST *> args;
    while (curr_token != '>') {
      args.push_back(parse_type());
      if (curr_token == '>')
        break;
      eat(',');
    }
    eat('>');
    return new GenericAccessAST(id, args);
  } else
    return new NamedTypeAST(parse_identifier(id));
}

TypeDefAST *parse_type_definition() {
  eat(T_TYPE);
  std::string name = identifier_string;
  eat(T_IDENTIFIER);
  std::vector<std::string> generics;
  bool is_generic = false;
  if (curr_token == '<') {
    is_generic = true;
    eat('<');
    while (curr_token != '>') {
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

TypeAST *parse_inline_struct_type(std::string first_name) {
  // switched from parse_tuple_type after parsing the first key and seeing a ':'
  std::vector<std::pair<std::string, TypeAST *>> fields;
  eat(':');
  auto first_type = parse_type();
  fields.push_back(std::make_pair(first_name, first_type));
  while (curr_token != '}') {
    eat(',');
    std::string name = identifier_string;
    eat(T_IDENTIFIER);
    eat(':');
    TypeAST *t = parse_type();
    fields.push_back(std::make_pair(name, t));
  }
  eat('}');
  return new StructTypeAST(fields);
}
TypeAST *parse_tuple_type() {
  eat('{');
  std::vector<TypeAST *> types;
  while (curr_token != '}') {
    auto type = parse_type();
    if (curr_token == ':') {
      auto named = dynamic_cast<NamedTypeAST *>(type);
      if (named && !named->name.has_spaces())
        return parse_inline_struct_type(named->name.name);
      else
        error("Unexpected : in tuple type");
    }
    types.push_back(type);
    if (curr_token == '}')
      break;
    eat(',');
  }
  eat('}');
  return new TupleTypeAST(types);
}
TypeAST *parse_primary_type() {
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
  case T_GENERIC: {
    eat(T_GENERIC);
    std::string name = identifier_string;
    eat(T_IDENTIFIER);
    return new GenericTypeAST(name);
  }
  }
  error("Unexpected token '" + token_to_str(curr_token) + "'");
}
TypeAST *parse_type_postfix() {
  TypeAST *prev = parse_primary_type();
  while (true) {
    switch (curr_token) {
    case '[': {
      eat('[');
      if (curr_token == ']') {
        eat(']');
        prev = new UnaryTypeAST('*', prev);
      } else if (curr_token == T_GENERIC) {
        eat(T_GENERIC);
        std::string name = identifier_string;
        eat(T_IDENTIFIER);
        eat(']');
        prev = new GenericArrayTypeAST(prev, name);
      } else {
        uint len = std::stoi(num_value, nullptr, num_base);
        if (num_has_dot)
          error("List lengths have to be integers");
        eat(T_NUMBER);
        eat(']');
        prev = new ArrayTypeAST(prev, len);
      }
      break;
    }
    case '|': {
      static bool pass = false;
      if (pass)
        return prev;
      eat('|');
      pass = true;
      std::vector<TypeAST *> types = {prev};
      while (1) {
        types.push_back(parse_type());
        if (curr_token != '|')
          break;
        eat('|');
      }
      pass = false;
      prev = new UnionTypeAST(types);
      break;
    }
    default:
      return prev;
    }
  }
}
TypeAST *parse_type_unary() {
  // If the current token is not a unary type operator, just parse type
  if (type_unaries.count(curr_token) == 0)
    return parse_type_postfix();
  // if it is a unary type operator, parse it
  int opc = curr_token;
  eat(opc);
  TypeAST *operand = parse_type_unary();
  return new UnaryTypeAST(opc, operand);
}
NumberExprAST *parse_number_expr() {
  auto result = new NumberExprAST(num_value, num_type, num_has_dot, num_base);
  eat(T_NUMBER);
  return result;
}
CharExprAST *parse_char_expr() {
  auto result = new CharExprAST(char_value);
  eat(T_CHAR);
  return result;
}

#include "asts/asts/string.h"
template <typename To>
ExprAST *convert_string(std::string str, StringType type) {
  std::basic_string<To> converted_str =
      std::wstring_convert<
          deletable_facet<std::codecvt<To, char, std::mbstate_t>>, To>{}
          .from_bytes(str);
  if (type == C_STRING)
    return new PtrStringExprAST<To>(converted_str, true);
  if (type == PTR_CHAR_ARRAY)
    return new PtrStringExprAST<To>(converted_str, false);
  else
    return new StringExprAST<To>(converted_str);
}

ExprAST *parse_string_expr() {
  auto type = string_type;
  std::string str = eat_string();
  if (curr_token == T_NUMBER) {
    auto num = std::stoi(num_value, nullptr, num_base);
    eat(T_NUMBER);
    switch (num) {
    case 8:
      return convert_string<char>(str, type);
    case 16:
      return convert_string<char16_t>(str, type);
    default:
      error("Unknown string size: " << num);
    }
  } else
    return convert_string<char>(str, type);
}
#include "asts/asts/tuple.h"
#include <vector>

TupleExprAST *parse_tuple_expr() {
  eat('(');
  std::vector<ExprAST *> exprs;
  while (curr_token != ')') {
    auto expr = parse_expr();
    exprs.push_back(expr);
    if (curr_token == ')')
      break;
    eat(',');
  }
  eat(')');
  return new TupleExprAST(exprs);
}
/// parenexpr ::= '(' expression ')'
ExprAST *parse_paren_expr() {
  eat('(');
  if (curr_token == ')') {
    eat(')');
    return new TupleExprAST({}); // empty tuple
  }
  auto expr = parse_expr();
  if (curr_token == ',') {
    eat(',');
    std::vector<ExprAST *> exprs;
    exprs.push_back(expr);
    while (curr_token != ')') {
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

ArrayExprAST *parse_array_expr() {
  eat('[');
  std::vector<ExprAST *> elements;
  while (curr_token != ']') {
    elements.push_back(parse_expr());
    if (curr_token == ']')
      break;
    eat(',');
  }
  eat(']');
  return new ArrayExprAST(elements);
}

#include "asts/asts/call.h"
#include "asts/asts/variable.h"
/// identifierexpr
///   ::= identifier
ExprAST *parse_identifier_expr() {
  auto id = parse_identifier();
  if (curr_token == '(') {
    std::vector<ExprAST *> args = parse_call();
    return new NameCallExprAST(id, args);
  }
  return new VariableExprAST(id);
}

#include "asts/asts/conditional.h"
#include "asts/asts/type.h"
/// ifexpr
///   ::= 'if' (expression) expression ('else' expression)?
///   ::= 'if' ('type' type == type) expression ('else' expression)?
ExprAST *parse_if_expr() {
  eat(T_IF);
  bool paren = curr_token == '(';
  if (paren)
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
  if (paren)
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
WhileExprAST *parse_while_expr() {
  eat(T_WHILE);
  auto cond = parse_expr();
  auto then = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  return new WhileExprAST(cond, then, elze);
}
/// forexpr ::= 'for' (expr; expr; expr) expr else expr
ForExprAST *parse_for_expr() {
  eat(T_FOR);
  bool paren = curr_token == '(';
  if (paren)
    eat('(');
  auto init = parse_expr(); // let i = 0
  eat(';');                 // ;
  auto cond = parse_expr(); // i < 5
  eat(';');                 // ;
  auto post = parse_expr(); // i = i + 1
  if (paren)
    eat(')');
  auto body = parse_expr();
  ExprAST *elze = nullptr;
  if (curr_token == T_ELSE) {
    eat(T_ELSE);
    elze = parse_expr();
  }
  return new ForExprAST(init, cond, body, post, elze);
}

#include "asts/asts/new.h"
/// newexpr ::= 'new|create' type '{' (identifier '=' expr ',')* '}'
ExprAST *parse_new_expr() {
  bool is_new = curr_token == T_NEW;
  eat(is_new ? T_NEW : T_CREATE);

  if (is_new && curr_token == '(') {
    auto tuple = parse_tuple_expr();
    tuple->is_new = true;
    return tuple;
  }
  auto type = parse_primary_type();
  std::vector<std::pair<std::string, ExprAST *>> fields;
  eat('{');
  while (curr_token != '}') {
    std::string key = "";
    ExprAST *value;
    if (curr_token == T_IDENTIFIER) {
      key = identifier_string;
      eat(T_IDENTIFIER);
      if (curr_token == '=')
        eat('=');
      else {
        // create X { a, b }
        key = "";
        value = new VariableExprAST(key);
        goto postvalue;
      }
    }
    value = parse_expr();
  postvalue:
    fields.push_back(std::make_pair(key, value));
    if (curr_token == '}')
      break;
    eat(',');
  }
  eat('}');
  return new NewExprAST(type, fields, is_new);
}

BlockExprAST *parse_block() {
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

LetExprAST *parse_let_expr() {
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

SizeofExprAST *parse_sizeof_expr() {
  eat(T_SIZEOF);
  bool paren = curr_token == '(';
  if (paren)
    eat('(');
  TypeAST *type = parse_type();
  if (paren)
    eat(')');
  return new SizeofExprAST(type);
}

BoolExprAST *parse_bool_expr() {
  bool val = curr_token == T_TRUE;
  eat(curr_token);
  return new BoolExprAST(val);
}

ExprAST *parse_type_assertion() {
  eat(T_ASSERT_TYPE);
  TypeAST *a = parse_type();
  eat(T_EQEQ);
  TypeAST *b = parse_type();
  return new TypeAssertExprAST(a, b);
}

ExprAST *parse_type_dump() {
  eat(T_DUMP);
  TypeAST *a = parse_type();
  return new TypeDumpExprAST(a);
}

std::vector<std::pair<std::string, ExprAST *>> parse_asm_expr_params() {
  std::vector<std::pair<std::string, ExprAST *>> params;
  eat('(');
  while (curr_token != ')') {
    std::string reg = identifier_string;
    eat(T_IDENTIFIER);
    eat('=');
    auto value = parse_expr();
    params.push_back(std::make_pair(reg, value));
    if (curr_token == ')')
      break;
    eat(',');
  }
  eat(')');
  return params;
}
// asmexpr ::= '__asm__' type '(' asm_str '=>' reg ')' '(' (reg '=' expr)* ')'
ASMExprAST *parse_asm_expr() {
  eat(T_ASM);
  if (curr_token == '(') {
    // side effect asm
    eat('(');
    std::string asm_str = eat_string();
    eat(')');
    auto args = parse_asm_expr_params();
    return new ASMExprAST(asm_str, args);
  }
  TypeAST *type = parse_type();
  eat('(');
  std::string asm_str = eat_string();
  eat('=');
  eat('>');
  std::string output_reg = identifier_string;
  eat(T_IDENTIFIER);
  eat(')');
  auto args = parse_asm_expr_params();
  return new ASMExprAST(type, asm_str, output_reg, args);
}

GlobalASMExprAST *parse_global_asm() {
  eat(T_ASM);
  eat('(');
  std::string asm_str = eat_string();
  eat(')');
  return new GlobalASMExprAST(asm_str);
}

#include "asts/asts/null.h"
/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
ExprAST *parse_primary() {
  switch (curr_token) {
  default:
    error("unexpected token '" + token_to_str(curr_token) +
          "' when expecting a primary");
  case T_IDENTIFIER:
    return parse_identifier_expr();
  case T_NUMBER:
    return parse_number_expr();
  case T_CHAR:
    return parse_char_expr();
  case T_NULL:
    eat(T_NULL);
    return new NullExprAST();
  case T_CONTINUE:
    eat(T_CONTINUE);
    return new ContinueExprAST();
  case T_BREAK:
    eat(T_BREAK);
    return new BreakExprAST();
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
  case T_ASM:
    return parse_asm_expr();
  case '{':
    return parse_block();
  case '[':
    return parse_array_expr();
  }
}
int get_token_precedence() {
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
  while (curr_token != ')') {
    args.push_back(parse_expr());
    if (curr_token == ')')
      break;
    eat(',');
  }
  eat(')');
  return args;
}
#include "asts/asts/index.h"
ExprAST *parse_postfix() {
  ExprAST *prev = parse_primary();
  while (true)
    switch (curr_token) {
    default:
      return prev;
    case '.': {
      eat('.');
      if (curr_token == T_NUMBER) {
        uint idx = std::stoi(num_value, nullptr, num_base);
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
    }
}

#include "asts/asts/unary.h"
/// unary
///   ::= primary
///   ::= '!' unary
ExprAST *parse_unary() {
  if (unaries.count(curr_token) == 0) // not a unary op
    return parse_postfix();

  int opc = curr_token;
  eat(opc);
  ExprAST *operand = parse_unary();
  return new UnaryExprAST(opc, operand);
}

#include "asts/asts/cast.h"
// parse low-priority postfix expressions
ExprAST *parse_lp_postfix() {
  ExprAST *prev = parse_unary();
  while (true)
    switch (curr_token) {
    default:
      return prev;
    case T_AS: {
      // cast
      eat(T_AS);
      TypeAST *cast_to = parse_type();
      prev = new CastExprAST(prev, cast_to);
      break;
    }
    }
}

#include "asts/asts/assign.h"
#include "asts/asts/binop.h"
#define parse_binop_side() parse_lp_postfix()
/// binoprhs
///   ::= ('+' primary)*
ExprAST *parse_bin_op_rhs(int expr_prec, ExprAST *LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    if (!binop_precedence.count(curr_token))
      return LHS; // doesn't exist
    int t_prec = binop_precedence[curr_token];
    // If this is a binop that binds at least as tightly as the current
    // binop, consume it, otherwise we are done.
    if (t_prec < expr_prec)
      return LHS;

    // Okay, we know this is a binop.
    int op = curr_token;
    eat(op);
    // Parse the primary expression after the binary operator.
    auto RHS = parse_binop_side();

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int next_prec = get_token_precedence();
    if (t_prec < next_prec)
      RHS = parse_bin_op_rhs(t_prec + 1, RHS);

    // Merge LHS/RHS.
    if (op == '=')
      LHS = new AssignExprAST(LHS, RHS);
    else if (op == T_OR)
      LHS = new OrExprAST(LHS, RHS);
    else if (op == T_AND)
      LHS = new AndExprAST(LHS, RHS);
    else if (op_eq_ops.count(op))
      // left = left op right
      LHS = new AssignExprAST(LHS, new BinaryExprAST(op_eq_ops[op], LHS, RHS));
    else
      LHS = new BinaryExprAST(op, LHS, RHS);
  }
}
ExprAST *parse_expr() { return parse_bin_op_rhs(0, parse_binop_side()); }

std::tuple<std::string, TypeAST *, FuncFlags>
parse_prototype_begin(bool parse_name, bool parse_this) {
  FuncFlags flags;
  flags.is_inline = curr_token == T_INLINE;
  if (flags.is_inline)
    eat(T_INLINE);
  eat(T_FUNCTION); // fun
  TypeAST *this_t = nullptr;
  if (parse_this && curr_token == '(') { // (*Thing)
    // type method
    eat('(');
    this_t = parse_type();
    eat(')');
  }
  if (curr_token == '[') {
    // function flags/attrs
    eat('[');
    eat('[');
    while (curr_token != ']') {
      std::string type = identifier_string;
      eat(T_IDENTIFIER);
      if (!flags.set_flag(type))
        error("Unknown flag type: " << type);
    }
    eat(']');
    eat(']');
  }
  std::string fn_name = identifier_string; // do_stuff
  if (parse_name)
    eat(T_IDENTIFIER);
  while (curr_token != '(') {
    if (curr_token == T_IDENTIFIER) {
      std::string type = identifier_string;
      eat(T_IDENTIFIER);
      eat('(');
      std::string str;
      if (curr_token == T_STRING)
        str = string_value;
      else if (curr_token == T_IDENTIFIER)
        str = identifier_string;
      else
        str = token_to_str(curr_token);
      if (!flags.set_by_string(type, str))
        error("Unknown flag type: " << type);
      get_next_token();
      eat(')');
    } else
      error("expected flag or '(' after function name");
  }
  return {fn_name, this_t, flags};
}

/// prototype
///   ::= fun id '(' id* ')'
///   ::= fun '(' type ')' id '(' id* ')'
FunctionAST *parse_prototype(TypeAST *default_return_type) {
  auto [fn_name, this_t, flags] = parse_prototype_begin(true, true);
  eat('(');

  // Read the list of argument names.
  std::vector<std::pair<std::string, TypeAST *>> args;

  if (curr_token != ')')
    while (1) {
      if (curr_token == T_VARARG) {
        eat(T_VARARG);
        flags.is_vararg = true;
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
    return new MethodAST(this_t, fn_name, args, flags, return_type);
  else
    return new FunctionAST(fn_name, args, flags, return_type);
}

/// definition ::= 'fun' prototype expression
FunctionAST *parse_definition() {
  FunctionAST *func = parse_prototype(nullptr /* assume from body */);
  func->body = parse_expr();
  return func;
}
/// declare
///   ::= 'fun' prototype
///   ::= 'let' identifier ':' type
DeclareExprAST *parse_declare() {
  eat(T_DECLARE); // eat declare.
  if (curr_token == T_FUNCTION) {
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
TypeDefAST *parse_struct() {
  eat(T_STRUCT); // eat struct.
  std::string struct_name = identifier_string;
  eat(T_IDENTIFIER);
  std::vector<std::string> generic_params;
  bool is_generic = false;
  if (curr_token == '<') {
    is_generic = true;
    eat('<');
    while (curr_token != '>') {
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
  while (curr_token != '}') {
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
                                 new StructTypeAST(members));
  else
    return new AbsoluteTypeDefAST(struct_name,
                                  new NamedStructTypeAST(struct_name, members));
}

/// include ::= 'include' string, make sure to eat(T_STRING) after calling!
std::string parse_include() {
  eat(T_INCLUDE);
  std::string path = string_value;
  if (curr_token != T_STRING) {
    error("Unexpected token after 'include': '" + token_to_str(curr_token) +
          "', expected string");
  }
  return path;
}