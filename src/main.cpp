#include "consts.cpp"
#include "utils.cpp"

#include "parser.cpp"
static void handle_definition() {
  if (auto ast = parse_definition()) {
    fprintf(stderr, "Parsed a function definition.\n");
    ast->gen_toplevel();
  } else
    // Skip token for error recovery.
    get_next_token();
}

static void handle_extern() {
  if (auto ast = parse_extern()) {
    fprintf(stderr, "Parsed an extern\n");
    ast->gen_toplevel();
  } else
    // Skip token for error recovery.
    get_next_token();
}
static void handle_global_let() {
  if (auto ast = parse_let_expr(true)) {
    fprintf(stderr, "Parsed a global variable\n");
    ast->gen_toplevel();
  } else
    // Skip token for error recovery.
    get_next_token();
}
static void handle_global_struct() {
  if (auto ast = parse_struct()) {
    fprintf(stderr, "Parsed a struct definition\n");
    ast->gen_toplevel();
  } else
    // Skip token for error recovery.
    get_next_token();
}

/// top ::= definition | external | expression | ';'
static void main_loop() {
  get_next_token();
  while (1) {
    switch (curr_token) {
    case T_EOF:
      return;
    case ';': // ignore top-level semicolons.
      get_next_token();
      break;
    case T_FUNCTION:
      handle_definition();
      break;
    case T_EXTERN:
      handle_extern();
      break;
    case T_CONST:
    case T_LET:
      handle_global_let();
      break;
    case T_STRUCT:
      handle_global_struct();
      break;
    default:
      fprintf(stderr, "Unexpected token '%c' (%d) at top-level", curr_token,
              curr_token);
      exit(1);
      break;
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s <filename> <output>\n", argv[0]);
    return 1;
  }

  // host machine triple
  char *target_triple = LLVMGetDefaultTargetTriple();
  LLVMTargetRef target;
  char *error_message;
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  if (LLVMGetTargetFromTriple(target_triple, &target, &error_message) != 0)
    error(error_message);
  char *host_cpu_name = LLVMGetHostCPUName();
  char *host_cpu_features = LLVMGetHostCPUFeatures();
  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, target_triple, host_cpu_name, host_cpu_features,
      LLVMCodeGenLevelAggressive, LLVMRelocStatic, LLVMCodeModelSmall);
  target_data = LLVMCreateTargetDataLayout(target_machine);
  // create module
  curr_module = LLVMModuleCreateWithName(argv[1]);
  // set target to current machine
  LLVMSetTarget(curr_module, target_triple);
  LLVMSetModuleDataLayout(curr_module, target_data);
  // create builder, context, and pass manager (for optimization)
  curr_builder = LLVMCreateBuilder();
  curr_ctx = LLVMGetGlobalContext();

  // open .fy file
  current_file = fopen(argv[1], "r");
  if (!current_file) {
    fprintf(stderr, "File \'%s\' doesn't exist", argv[1]);
    return 1;
  }
  // parse and compile everything into LLVM IR
  main_loop();
  // export LLVM IR into other file
  char *err_msg;
  char *output = LLVMPrintModuleToString(curr_module);
  FILE *output_file = fopen(argv[2], "w");
  fprintf(output_file, "%s", output);
  // dispose of a bunch of stuff
  LLVMDisposeMessage(output);
  LLVMDisposeModule(curr_module);
  LLVMDisposeBuilder(curr_builder);
  LLVMDisposeMessage(target_triple);
  LLVMDisposeMessage(host_cpu_name);
  LLVMDisposeMessage(host_cpu_features);
  LLVMDisposeTargetMachine(target_machine);
  return 0;
}