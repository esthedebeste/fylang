#include "utils.h"
#include <cmath>

size_t unnamed_acc = 0;
// incrementing base52 (a-zA-Z) number for unnamed symbols
const char *next_unnamed() {
  size_t num = unnamed_acc++;
  if (num == 0)
    return "a"; // log(0) would fail so shortcut with correct result
  size_t len = log(num) / log(52) + 1;
  char *buf = new char[len + 1];
  for (size_t i = len; i > 0; i--) {
    unsigned short curr = num % 52;
    num /= 52;
    buf[i - 1] = curr < 26 ? curr + 'a' : curr + 'A' - 26;
  }
  buf[len] = '\0';
  return buf;
}

LLVMCallConv get_call_conv(std::string name) {
  if (call_convs.count(name))
    return call_convs[name];
  if (name == "EFIAPI")
    return LLVMPointerSize(target_data) == 8 ? LLVMWin64CallConv // 64-bit efi
                                             : LLVMCCallConv;    // 32-bit efi
  else
    error("unknown call convention: " + name);
}
bool FuncFlags::set_by_string(std::string str, std::string value) {
  if (str == "call_conv" || str == "cc")
    call_conv = get_call_conv(value);
  else {
    // boolean flags
    bool enabled = value == "true";
    if (str == "vararg")
      is_vararg = enabled;
    else if (str == "inline")
      is_inline = enabled;
    // might rename to "export" or "extern"? not sure.
    else if (str == "always_compile")
      always_compile = enabled;
    else
      return false;
  }
  return true;
}
bool FuncFlags::set_flag(std::string str) {
  if (str == "vararg")
    return is_vararg = true;
  else if (str == "inline")
    return is_inline = true;
  // might rename to "export" or "extern"? not sure.
  else if (str == "always_compile")
    return always_compile = true;
  return false;
}
bool FuncFlags::eq(FuncFlags other) {
  return is_vararg == other.is_vararg && is_inline == other.is_inline &&
         always_compile == other.always_compile && call_conv == other.call_conv;
}
bool FuncFlags::neq(FuncFlags other) { return !eq(other); }
#if defined(_WIN32)
#include <Windows.h>
#include <wchar.h>
std::filesystem::path get_executable_path() {
  wchar_t buffer[4096];
  GetModuleFileNameW(NULL, buffer, sizeof(buffer));
  return std::filesystem::path(buffer).parent_path();
}
#elif defined(__linux__)
#include <unistd.h>
std::filesystem::path get_executable_path() {
  char buffer[4096];
  ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (len == -1)
    return "";
  return std::filesystem::path(std::string(buffer, len)).parent_path();
}
#else
std::filesystem::path get_executable_path() {
  static_assert(false, "Unsupported platform (expected _WIN32 or __linux__)");
}
#endif