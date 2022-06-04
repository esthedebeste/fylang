#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
extern "C" {
#include "llvm-c-14/llvm-c/BitWriter.h"
#include "llvm-c-14/llvm-c/Core.h"
#include "llvm-c-14/llvm-c/ExecutionEngine.h"
#include "llvm-c-14/llvm-c/TargetMachine.h"
}
extern bool DEBUG;
extern std::string std_dir;
extern std::string os_name;
using uint = unsigned int;
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
  T_LSHIFT,        // <<
  T_RSHIFT,        // >>
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
  T_ASM,           // __asm__
  T_OR,            // or
};

extern LLVMContextRef curr_ctx;
extern LLVMBuilderRef curr_builder;
extern LLVMModuleRef curr_module;
extern LLVMTargetDataRef target_data;
extern std::unordered_map<int, int> binop_precedence;
#define assign_prec 1
#define logical_prec 5
#define comparison_prec 10
#define add_prec 20
#define mul_prec 40
#define binary_op_prec 60

extern std::unordered_map<int, int> op_eq_ops;
extern std::unordered_map<Token, std::string> token_strs;
extern std::unordered_map<std::string, Token> keywords;

extern std::unordered_set<int> unaries;
extern std::unordered_set<int> type_unaries;
extern std::unordered_map<std::string, LLVMCallConv> call_convs;