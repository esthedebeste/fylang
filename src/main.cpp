#include "parser.cpp"
#include "ucr.cpp"
#include "utils.cpp"
static void handle_definition() {
  auto ast = parse_definition();
  debug_log("Parsed a function definition (name: " << ast->name << ")");
  ast->add();
}

static void handle_declare() {
  auto ast = parse_declare();
  debug_log("Parsed a declare\n");
  auto val = ast->gen_toplevel();
  if (DEBUG && val)
    LLVMDumpValue(val);
}
static void handle_global_let() {
  auto ast = parse_let_expr();
  debug_log("Parsed a global variable\n");
  auto val = ast->gen_toplevel();
  if (DEBUG)
    LLVMDumpValue(val);
}
static void handle_global_struct() {
  auto ast = parse_struct();
  debug_log("Parsed a struct definition\n");
  ast->gen_toplevel();
}
static void handle_global_type() {
  auto ast = parse_type_definition();
  debug_log("Parsed a type definition\n");
  ast->gen_toplevel();
}
static void handle_global_include() {
  std::string file_name = parse_include();
  debug_log("Parsed an include (" << file_name << ")");
  size_t os_loc = file_name.find("{os}");
  if (os_loc != std::string::npos) {
    file_name = file_name.replace(os_loc, 4, os_name);
    debug_log("Replaced {os} with '" << os_name << "'");
  }
  CharReader *curr_file = queue.back();
  add_file_to_queue(curr_file->file_path, file_name);
  eat(T_STRING);
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
    case T_INLINE:
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
      error("Unexpected token '" + token_to_str(curr_token) + "' at top-level");
    }
  }
}

std::string get_os(std::string triple) {
  // triple: "x86_64-unknown-linux-gnu"
  // slice away first two parts, triple: "linux-gnu"
  size_t start = triple.find('-', triple.find('-') + 1) + 1;
  // slice away last part, triple: "linux"
  return triple.substr(start, triple.find('-', start) - start);
}

int main(int argc, char **argv, char **envp) {
  if (argc < 2) {
    printf("Usage: %s [run|com] <filename> (output)\n", argv[0]);
    return 1;
  }
  char *input = argv[2];
  std::string mode_str = argv[1];
  enum { COMPILE, RUN } mode;
  if (mode_str == "run")
    mode = RUN;
  else if (mode_str == "com")
    mode = COMPILE;
  else {
    printf("Usage: %s [run|com] <filename> (output)\n", argv[0]);
    return 1;
  }

  if (getenv("DEBUG"))
    DEBUG = true;
  bool QUIET = getenv("QUIET");
  std_dir = get_executable_path().append("../lib").string();
  // host machine triple
  char *target_triple = LLVMGetDefaultTargetTriple();
  LLVMTargetRef target;
  char *error_message;
  LLVMInitializeNativeTarget();
  if (LLVMGetTargetFromTriple(target_triple, &target, &error_message) != 0)
    error(error_message);
  os_name = get_os(target_triple);
  char *host_cpu_name = LLVMGetHostCPUName();
  char *host_cpu_features = LLVMGetHostCPUFeatures();
  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, target_triple, host_cpu_name, host_cpu_features,
      LLVMCodeGenLevelAggressive, LLVMRelocStatic, LLVMCodeModelSmall);
  target_data = LLVMCreateTargetDataLayout(target_machine);
  // create module
  curr_module = LLVMModuleCreateWithName(input);
  // set target to current machine
  LLVMSetTarget(curr_module, target_triple);
  LLVMSetModuleDataLayout(curr_module, target_data);
  // create builder, context, and pass manager (for optimization)
  curr_builder = LLVMCreateBuilder();
  curr_ctx = LLVMGetGlobalContext();
  // open .fy file
  add_file_to_queue(".", input);
  // parse and compile everything into LLVM IR
  main_loop();
  LLVMValueRef main_function =
      curr_named_functions.count("main")
          ? curr_named_functions["main"]->gen_ptr()->gen_val()
          : nullptr;
  std::vector<LLVMValueRef> entry_functions;
  if (main_function)
    entry_functions.push_back(main_function);
  for (auto &[_, func] : curr_named_functions)
    if (func->flags.always_compile)
      entry_functions.push_back(func->gen_ptr()->gen_val());
  if (!getenv("NO_UCR") && entry_functions.size() > 0)
    remove_unused_globals(curr_module, entry_functions);
  if (main_function)
    add_stores_before_main(main_function);
  if (mode == COMPILE) {
    std::string out = argv[3];
    size_t ext_pos = out.rfind('.');
    size_t slash_pos = out.rfind('/');
    std::string ext = ext_pos == std::string::npos || slash_pos > ext_pos
                          ? "ll" // default to LLVM IR
                          : out.substr(ext_pos + 1);
    char *err = nullptr;
    // export LLVM IR into other file
    if (ext == "bc")
      LLVMWriteBitcodeToFile(curr_module, out.c_str());
    else if (ext == "asm")
      LLVMTargetMachineEmitToFile(target_machine, curr_module,
                                  strdup(out.c_str()), LLVMAssemblyFile, &err);
    else if (ext == "o")
      LLVMTargetMachineEmitToFile(target_machine, curr_module,
                                  strdup(out.c_str()), LLVMObjectFile, &err);
    else if (ext == "ll")
      LLVMPrintModuleToFile(curr_module, out.c_str(), &err);
    else
      error("Unknown file extension: " + ext);

    if (err)
      error(err);
    if (!QUIET)
      std::cout << "\n\033[32m[fy] Successfully compiled " << input << " to "
                << out << "\n\033[0m" << std::endl;
    return 0;
  } else if (mode == RUN) {
    if (!main_function)
      error("No main function found, cannot run");
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMLinkInMCJIT();
    LLVMExecutionEngineRef engine;
    char *err;
    bool errored =
        LLVMCreateJITCompilerForModule(&engine, curr_module, 0, &err);
    if (errored)
      error(std::string("JIT Failed: ") + err);
    int nargc = argc - 2;
    char **nargv = argv + 2;
    int exit_code =
        LLVMRunFunctionAsMain(engine, main_function, nargc, nargv, envp);
    if (!QUIET)
      std::cout << "\n\033[32m[fy] Executed with exit code " << exit_code
                << "\n\033[0m" << std::endl;
    return exit_code;
  }
  error("Unreachable");
}