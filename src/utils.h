#pragma once
#include "consts.h"
#include <filesystem>
#include <iostream>
#include <vector>

template <typename A, typename B>
std::vector<B> seconds(const std::vector<std::pair<A, B>> &array) {
  size_t len = array.size();
  std::vector<B> res(len);
  for (size_t i = 0; i < len; ++i)
    res[i] = array[i].second;
  return res;
}

std::string token_to_str(const int token);
extern size_t unnamed_acc;
const char *next_unnamed();
// Unnamed symbol
#define UN next_unnamed()

consteval const char *__file_name__(const char *path) {
  const char *last = path;
  while (*path)
    if (*path++ == '/')
      last = path;
  return last;
}

#define error(err) (std::cerr << "Error: " << err << std::endl), exit(1)
#define debug_log(format)                                                      \
  if (DEBUG)                                                                   \
  std::cerr << "[" << __file_name__(__FILE__) << ":" << __LINE__ << "] "       \
            << format << std::endl

LLVMCallConv get_call_conv(std::string name);

struct FuncFlags {
  bool is_vararg = false, // is the function vararg
      is_inline = false,  // should instructions be inlined into the call-site
      always_compile = false; // should the function be compiled even if it
                              // isn't referenced
  LLVMCallConv call_conv = LLVMCCallConv; // calling convention
  bool set_by_string(std::string str, std::string value);
  bool set_flag(std::string str);
  bool eq(FuncFlags other);
  bool neq(FuncFlags other);
};

// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template <class Facet> struct deletable_facet : Facet {
  template <class... Args>
  deletable_facet(Args &&...args) : Facet(std::forward<Args>(args)...) {}
  ~deletable_facet() {}
};

std::filesystem::path get_executable_path();