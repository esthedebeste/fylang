#include "utils.cpp"

#include "parser.cpp"
#include "ucr.cpp"
static void handle_definition() {
  auto ast = parse_definition();
  if (DEBUG)
    fprintf(stderr, "Parsed a function definition (name: %s).\n",
            ast->proto->name);
  auto val = ast->gen_toplevel();
  if (DEBUG)
    LLVMDumpValue(val);
}

static void handle_declare() {
  auto ast = parse_declare();
  if (DEBUG)
    fprintf(stderr, "Parsed a declare\n");
  auto val = ast->gen_toplevel();
  if (DEBUG)
    LLVMDumpValue(val);
}
static void handle_global_let() {
  auto ast = parse_let_expr(true);
  if (DEBUG)
    fprintf(stderr, "Parsed a global variable\n");
  auto val = ast->gen_toplevel();
  if (DEBUG)
    LLVMDumpValue(val);
}
static void handle_global_struct() {
  auto ast = parse_struct();
  if (DEBUG)
    fprintf(stderr, "Parsed a struct definition\n");
  ast->gen_toplevel();
}
static void handle_global_type() {
  auto ast = parse_type_definition();
  if (DEBUG)
    fprintf(stderr, "Parsed a type definition\n");
  ast->gen_toplevel();
}
static void handle_global_include() {
  char *file_name = parse_include();
  if (DEBUG)
    fprintf(stderr, "Parsed an include\n");
  CharReader *curr_file = queue[queue_len - 1];
  add_file_to_queue(curr_file->file_path, file_name);
  get_next_token();
}

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
    case T_DECLARE:
      handle_declare();
      break;
    case T_CONST:
    case T_LET:
      handle_global_let();
      break;
    case T_STRUCT:
      handle_global_struct();
      break;
    case T_INCLUDE:
      handle_global_include();
      break;
    case T_TYPE:
      handle_global_type();
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

  if (getenv("DEBUG"))
    DEBUG = true;
  std_dir = get_relative_path(argv[0], (char *)"../lib");
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
  add_file_to_queue((char *)".", argv[1]);
  // parse and compile everything into LLVM IR
  main_loop();
  remove_unused_globals(curr_module, LLVMGetNamedFunction(curr_module, "main"));
  // export LLVM IR into other file
  char *output = LLVMPrintModuleToString(curr_module);
  FILE *output_file = fopen(argv[2], "w");
  fprintf(output_file, "%s", output);
  fprintf(stderr, "\nSuccessfully compiled %s to %s\n", argv[1], argv[2]);
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