#pragma once
#include <ctype.h>
#include <libgen.h>
#include <malloc.h>
#include <map>
#include <math.h>
#include <memory>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
extern "C" {
#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Transforms/PassBuilder.h"
}

bool DEBUG = false;
char *std_dir;
enum Tokens : const int {
  T_EOF = -1,        // end of file
  T_IDENTIFIER = -2, // foo
  T_NUMBER = -3,     // 123
  T_STRING = -4,     // "foo"
  T_CHAR = -5,       // 'a'
  T_BOOL = -6,       // true
  T_IF = -7,         // if
  T_ELSE = -8,       // else
  T_WHILE = -9,      // while
  T_RETURN = -10,    // return
  T_FUNCTION = -11,  // fun
  T_DECLARE = -12,   // declare
  T_LET = -13,       // let
  T_CONST = -14,     // const
  T_STRUCT = -15,    // struct
  T_NEW = -16,       // new
  T_EQEQ = -17,      // ==
  T_LEQ = -18,       // <=
  T_GEQ = -19,       // >=
  T_NEQ = -20,       // !=
  T_LOR = -21,       // ||
  T_LAND = -22,      // &&
  T_INCLUDE = -23,   // include
  T_TYPE = -24,      // type
  T_UNSIGNED = -25,  // unsigned
  T_SIGNED = -26,    // signed
  T_AS = -27,        // as
  T_VARARG = -28,    // __VARARG__
  T_TYPEOF = -29,    // typeof
  T_TRUE = -30,      // true
  T_FALSE = -31,     // false
};

static LLVMContextRef curr_ctx;
static LLVMBuilderRef curr_builder;
static LLVMModuleRef curr_module;
static LLVMTargetDataRef target_data;
static std::map<int, int> binop_precedence = {
    {'=', 1},     {T_LOR, 5},  {T_LAND, 5}, {'<', 10},   {'>', 10},
    {T_EQEQ, 10}, {T_LEQ, 10}, {T_GEQ, 10}, {T_NEQ, 10}, {'+', 20},
    {'-', 20},    {'*', 40},   {'&', 60},   {'|', 60}};
