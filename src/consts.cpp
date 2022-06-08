#include "consts.h"
bool DEBUG = false;
std::string std_dir;
std::string os_name;
LLVMContextRef curr_ctx;
LLVMBuilderRef curr_builder;
LLVMModuleRef curr_module;
LLVMTargetDataRef target_data;

std::unordered_map<int, int> binop_precedence = {
#define assign_prec 1
    {'=', 1},       {T_PLUSEQ, 1},    {T_MINEQ, 1},   {T_STAREQ, 1},
    {T_SLASHEQ, 1}, {T_PERCENTEQ, 1}, {T_ANDEQ, 1},   {T_OREQ, 1},
#define logical_prec 5
    {T_LOR, 5},     {T_LAND, 5},      {T_OR, 5},
#define comparison_prec 10
    {'<', 10},      {'>', 10},        {T_EQEQ, 10},   {T_LEQ, 10},
    {T_GEQ, 10},    {T_NEQ, 10},
#define add_prec 20
    {'+', 20},      {'-', 20},
#define mul_prec 40
    {'*', 40},      {'/', 40},        {'%', 40},
#define binary_op_prec 60
    {'&', 60},      {'|', 60},        {T_LSHIFT, 60}, {T_RSHIFT, 60},
};

// += to +, -= to -, etc.
std::unordered_map<int, int> op_eq_ops = {
    {T_PLUSEQ, '+'},    {T_MINEQ, '-'}, {T_STAREQ, '*'}, {T_SLASHEQ, '/'},
    {T_PERCENTEQ, '%'}, {T_ANDEQ, '&'}, {T_OREQ, '|'},
};
std::unordered_map<Token, std::string> token_strs = {
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
    {T_LSHIFT, "<<"},
    {T_RSHIFT, ">>"},
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
    {T_ASM, "__asm__"},
    {T_OR, "or"},
};

std::unordered_map<std::string, Token> keywords = {
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
    {"__asm__", T_ASM},
    {"or", T_OR},
};

std::unordered_set<int> unaries = {'!', '~', '*', '&', '+', '-', T_RETURN};
std::unordered_set<int> type_unaries = {'*', '&', T_UNSIGNED, T_SIGNED};

// clang-format off
std::unordered_map<std::string, LLVMCallConv> call_convs = {{"C", LLVMCCallConv}, {"Fast", LLVMFastCallConv}, {"Cold", LLVMColdCallConv}, {"GHC", LLVMGHCCallConv}, {"HiPE", LLVMHiPECallConv}, {"WebKitJS", LLVMWebKitJSCallConv}, {"AnyReg", LLVMAnyRegCallConv}, {"PreserveMost", LLVMPreserveMostCallConv}, {"PreserveAll", LLVMPreserveAllCallConv}, {"Swift", LLVMSwiftCallConv}, {"CXXFASTTLS", LLVMCXXFASTTLSCallConv}, {"X86Stdcall", LLVMX86StdcallCallConv}, {"X86Fastcall", LLVMX86FastcallCallConv}, {"ARMAPCS", LLVMARMAPCSCallConv}, {"ARMAAPCS", LLVMARMAAPCSCallConv}, {"ARMAAPCSVFP", LLVMARMAAPCSVFPCallConv}, {"MSP430INTR", LLVMMSP430INTRCallConv}, {"X86ThisCall", LLVMX86ThisCallCallConv}, {"PTXKernel", LLVMPTXKernelCallConv}, {"PTXDevice", LLVMPTXDeviceCallConv}, {"SPIRFUNC", LLVMSPIRFUNCCallConv}, {"SPIRKERNEL", LLVMSPIRKERNELCallConv}, {"IntelOCLBI", LLVMIntelOCLBICallConv}, {"X8664SysV", LLVMX8664SysVCallConv}, {"Win64", LLVMWin64CallConv}, {"X86VectorCall", LLVMX86VectorCallCallConv}, {"HHVM", LLVMHHVMCallConv}, {"HHVMC", LLVMHHVMCCallConv}, {"X86INTR", LLVMX86INTRCallConv}, {"AVRINTR", LLVMAVRINTRCallConv}, {"AVRSIGNAL", LLVMAVRSIGNALCallConv}, {"AVRBUILTIN", LLVMAVRBUILTINCallConv}, {"AMDGPUVS", LLVMAMDGPUVSCallConv}, {"AMDGPUGS", LLVMAMDGPUGSCallConv}, {"AMDGPUPS", LLVMAMDGPUPSCallConv}, {"AMDGPUCS", LLVMAMDGPUCSCallConv}, {"AMDGPUKERNEL", LLVMAMDGPUKERNELCallConv}, {"X86RegCall", LLVMX86RegCallCallConv}, {"AMDGPUHS", LLVMAMDGPUHSCallConv}, {"MSP430BUILTIN", LLVMMSP430BUILTINCallConv}, {"AMDGPULS", LLVMAMDGPULSCallConv}, {"AMDGPUES", LLVMAMDGPUESCallConv}};
// clang-format on