#pragma once
extern "C"
{
#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Transforms/PassBuilder.h"
}

const int T_EOF = -1;        // end of file
const int T_IDENTIFIER = -2; // foo
const int T_NUMBER = -3;     // 123
const int T_STRING = -4;     // "foo"
const int T_CHAR = -5;       // 'a'
const int T_BOOL = -6;       // true
const int T_IF = -7;         // if
const int T_ELSE = -8;       // else
const int T_WHILE = -9;      // while
const int T_RETURN = -10;    // return
const int T_FUNCTION = -11;  // fun
const int T_EXTERN = -12;    // extern
const int T_LET = -13;       // let
const int T_EQEQ = -14;      // ==

static LLVMContextRef curr_ctx;
static LLVMBuilderRef curr_builder;
static LLVMModuleRef curr_module;
static LLVMPassManagerRef curr_pass_manager;
static LLVMTargetDataRef target_data;