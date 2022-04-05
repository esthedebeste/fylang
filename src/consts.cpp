#pragma once
#include <ctype.h>
#include <libgen.h>
#include <malloc.h>
#include <math.h>
#include <memory>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
extern "C" {
#include "llvm-c-15/llvm-c/Core.h"
#include "llvm-c-15/llvm-c/TargetMachine.h"
}
bool DEBUG = false;
bool QUIET = false;
char *std_dir;
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
  T_FOR,           // for
  T_PLUSEQ,        // +=
  T_MINEQ,         // -=
  T_STAREQ,        // *=
  T_SLASHEQ,       // /=
  T_PERCENTEQ,     // %=
  T_ANDEQ,         // &=
  T_OREQ,          // |=
  T_DUMP,          // DUMP
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