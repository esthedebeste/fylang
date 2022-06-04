#pragma once
#include "asts/asts.h"
#include "asts/functions.h"
#include "asts/types.h"
#include "lexer.h"

extern int curr_token;
int get_next_token();
void eat(const int expected_token);
std::string eat_string();
ExprAST *parse_expr();
ExprAST *parse_primary();
ExprAST *parse_unary();
std::vector<ExprAST *> parse_call();

TypeAST *parse_type_unary();
TypeAST *parse_type_postfix();
#define parse_type() parse_type_unary()
std::tuple<std::string, TypeAST *, FuncFlags>
parse_prototype_begin(bool parse_name, bool parse_this);
TypeAST *parse_function_type();
TypeAST *parse_num_type();
TypeDefAST *parse_type_definition();
TypeAST *parse_inline_struct_type(std::string first_name);
TypeAST *parse_tuple_type();
TypeAST *parse_primary_type();
TypeAST *parse_type_postfix();
TypeAST *parse_type_unary();
NumberExprAST *parse_number_expr();
CharExprAST *parse_char_expr();
ExprAST *parse_string_expr();
ExprAST *parse_paren_expr();
ExprAST *parse_identifier_expr();
ExprAST *parse_if_expr();
WhileExprAST *parse_while_expr();
ForExprAST *parse_for_expr();
ExprAST *parse_new_expr();
BlockExprAST *parse_block();
LetExprAST *parse_let_expr();
SizeofExprAST *parse_sizeof_expr();
BoolExprAST *parse_bool_expr();
ExprAST *parse_type_assertion();
ExprAST *parse_type_dump();
std::vector<std::pair<std::string, ExprAST *>> parse_asm_expr_params();
ExprAST *parse_asm_expr();
ExprAST *parse_primary();
ExprAST *parse_postfix();
ExprAST *parse_unary();
ExprAST *parse_bin_op_rhs(int expr_prec, ExprAST *LHS);
ExprAST *parse_expr();
FunctionAST *parse_prototype(TypeAST *default_return_type = nullptr);
FunctionAST *parse_definition();
DeclareExprAST *parse_declare();
TypeDefAST *parse_struct();
std::string parse_include();