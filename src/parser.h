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
TypeAST *parse_function_type();
TypeAST *parse_num_type();
#include "asts/asts/typedef.h"
TypeDefAST *parse_type_definition();
TypeAST *parse_inline_struct_type(std::string first_name);
TypeAST *parse_tuple_type();
TypeAST *parse_primary_type();
TypeAST *parse_type_postfix();
TypeAST *parse_type_unary();
#include "asts/asts/number.h"
NumberExprAST *parse_number_expr();
#include "asts/asts/char.h"
CharExprAST *parse_char_expr();
ExprAST *parse_string_expr();
ExprAST *parse_paren_expr();
ExprAST *parse_identifier_expr();
ExprAST *parse_if_expr();
#include "asts/asts/loop.h"
WhileExprAST *parse_while_expr();
ForExprAST *parse_for_expr();
ExprAST *parse_new_expr();
#include "asts/asts/block.h"
BlockExprAST *parse_block();
#include "asts/asts/array.h"
ArrayExprAST *parse_array_expr();
#include "asts/asts/let.h"
LetExprAST *parse_let_expr();
#include "asts/asts/sizeof.h"
SizeofExprAST *parse_sizeof_expr();
#include "asts/asts/bool.h"
BoolExprAST *parse_bool_expr();
ExprAST *parse_type_assertion();
ExprAST *parse_type_dump();
#include "asts/asts/asm.h"
ASMExprAST *parse_asm_expr();
GlobalASMExprAST *parse_global_asm();
ExprAST *parse_primary();
ExprAST *parse_postfix();
ExprAST *parse_unary();
ExprAST *parse_bin_op_rhs(int expr_prec, ExprAST *LHS);
ExprAST *parse_expr();
FunctionAST *parse_prototype(TypeAST *default_return_type = nullptr);
FunctionAST *parse_definition();
#include "asts/asts/declare.h"
DeclareExprAST *parse_declare();
TypeDefAST *parse_struct();
std::string parse_include();