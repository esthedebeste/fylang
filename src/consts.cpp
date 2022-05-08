#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
extern "C" {
#include "llvm-c-14/llvm-c/BitWriter.h"
#include "llvm-c-14/llvm-c/Core.h"
#include "llvm-c-14/llvm-c/ExecutionEngine.h"
#include "llvm-c-14/llvm-c/TargetMachine.h"
}
bool DEBUG = false;
std::string std_dir;
std::string os_name;
typedef unsigned int uint;
enum Token : const int {
  T_EOF = -0xffff, // end of file
  T_IDENTIFIER,    // foo
  T_NUMBER,        // 123
  T_STRING,        // "foo"
  T_CHAR,          // 'a'
  T_BOOL,          // true
  T_IF,            // if
  T_ELSE,          // else
  T_WHILE,         // while
  T_RETURN,        // return
  T_FUNCTION,      // fun
  T_DECLARE,       // declare
  T_LET,           // let
  T_CONST,         // const
  T_STRUCT,        // struct
  T_NEW,           // new
  T_CREATE,        // create
  T_EQEQ,          // ==
  T_LEQ,           // <=
  T_GEQ,           // >=
  T_NEQ,           // !=
  T_LOR,           // ||
  T_LAND,          // &&
  T_INCLUDE,       // include
  T_TYPE,          // type
  T_UNSIGNED,      // unsigned
  T_SIGNED,        // signed
  T_AS,            // as
  T_VARARG,        // __VARARG__
  T_TYPEOF,        // typeof
  T_SIZEOF,        // sizeof
  T_TRUE,          // true
  T_FALSE,         // false
  T_NULL,          // null
  T_FOR,           // for
  T_PLUSEQ,        // +=
  T_MINEQ,         // -=
  T_STAREQ,        // *=
  T_SLASHEQ,       // /=
  T_PERCENTEQ,     // %=
  T_ANDEQ,         // &=
  T_OREQ,          // |=
  T_DUMP,          // DUMP
  T_ASSERT_TYPE,   // ASSERT_TYPE
  T_GENERIC,       // generic
  T_INLINE,        // inline
};

static LLVMContextRef curr_ctx;
static LLVMBuilderRef curr_builder;
static LLVMModuleRef curr_module;
static LLVMTargetDataRef target_data;
static std::unordered_map<int, int> binop_precedence = {
#define assign_prec 1
    {'=', 1},       {T_PLUSEQ, 1},    {T_MINEQ, 1}, {T_STAREQ, 1},
    {T_SLASHEQ, 1}, {T_PERCENTEQ, 1}, {T_ANDEQ, 1}, {T_OREQ, 1},
#define logical_prec 5
    {T_LOR, 5},     {T_LAND, 5},
#define comparison_prec 10
    {'<', 10},      {'>', 10},        {T_EQEQ, 10}, {T_LEQ, 10},
    {T_GEQ, 10},    {T_NEQ, 10},
#define add_prec 20
    {'+', 20},      {'-', 20},
#define mul_prec 40
    {'*', 40},      {'/', 40},        {'%', 40},
#define binary_op_prec 60
    {'&', 60},      {'|', 60}};

// += to +, -= to -, etc.
static std::unordered_map<int, int> op_eq_ops = {
    {T_PLUSEQ, '+'},    {T_MINEQ, '-'}, {T_STAREQ, '*'}, {T_SLASHEQ, '/'},
    {T_PERCENTEQ, '%'}, {T_ANDEQ, '&'}, {T_OREQ, '|'},
};
static std::unordered_map<Token, std::string> token_strs = {
    {T_EOF, "EOF"},
    {T_IDENTIFIER, "identifier"},
    {T_NUMBER, "number"},
    {T_STRING, "string"},
    {T_CHAR, "char"},
    {T_BOOL, "boolean"},
    {T_IF, "if"},
    {T_ELSE, "else"},
    {T_WHILE, "while"},
    {T_RETURN, "return"},
    {T_FUNCTION, "fun"},
    {T_DECLARE, "declare"},
    {T_LET, "let"},
    {T_CONST, "const"},
    {T_STRUCT, "struct"},
    {T_NEW, "new"},
    {T_CREATE, "create"},
    {T_EQEQ, "=="},
    {T_LEQ, "<="},
    {T_GEQ, ">="},
    {T_NEQ, "!="},
    {T_LOR, "||"},
    {T_LAND, "&&"},
    {T_INCLUDE, "include"},
    {T_TYPE, "type"},
    {T_UNSIGNED, "unsigned"},
    {T_SIGNED, "signed"},
    {T_AS, "as"},
    {T_VARARG, "__VARARG__"},
    {T_TYPEOF, "typeof"},
    {T_SIZEOF, "sizeof"},
    {T_TRUE, "true"},
    {T_FALSE, "false"},
    {T_NULL, "null"},
    {T_FOR, "for"},
    {T_PLUSEQ, "+="},
    {T_MINEQ, "-="},
    {T_STAREQ, "*="},
    {T_SLASHEQ, "/="},
    {T_PERCENTEQ, "%="},
    {T_ANDEQ, "&="},
    {T_OREQ, "|="},
    {T_DUMP, "DUMP"},
    {T_ASSERT_TYPE, "ASSERT_TYPE"},
    {T_GENERIC, "generic"},
    {T_INLINE, "inline"},
};
static std::unordered_map<std::string, Token> keywords = {
    {"if", T_IF},
    {"else", T_ELSE},
    {"while", T_WHILE},
    {"return", T_RETURN},
    {"fun", T_FUNCTION},
    {"declare", T_DECLARE},
    {"let", T_LET},
    {"const", T_CONST},
    {"struct", T_STRUCT},
    {"new", T_NEW},
    {"create", T_CREATE},
    {"include", T_INCLUDE},
    {"type", T_TYPE},
    {"unsigned", T_UNSIGNED},
    {"signed", T_SIGNED},
    {"as", T_AS},
    {"__VARARG__", T_VARARG},
    {"typeof", T_TYPEOF},
    {"sizeof", T_SIZEOF},
    {"true", T_TRUE},
    {"false", T_FALSE},
    {"null", T_NULL},
    {"for", T_FOR},
    {"DUMP", T_DUMP},
    {"ASSERT_TYPE", T_ASSERT_TYPE},
    {"generic", T_GENERIC},
    {"inline", T_INLINE},
};

static std::unordered_set<int> unaries = {'!', '*', '&', '+', '-', T_RETURN};
static std::unordered_set<int> type_unaries = {'*', '&', T_UNSIGNED, T_SIGNED};