#pragma once
#include "consts.cpp"
// Shortcut for '(Elem*) calloc(amount, sizeof(Elem))'
template <typename Elem> inline Elem *alloc_arr(size_t amount) {
  return (Elem *)calloc(amount, sizeof(Elem));
}
// Shortcut for '(Elem*) reallocarray(ptr, amount, sizeof(Elem))'
template <typename Elem> inline Elem *realloc_arr(Elem *ptr, size_t amount) {
  return (Elem *)reallocarray(ptr, amount, sizeof(Elem));
}
inline char *alloc_c(size_t amount) { return alloc_arr<char>(amount); }
inline char *realloc_c(char *ptr, size_t amount) {
  return realloc_arr<char>(ptr, amount);
}

static bool streql(const char *a, const char *b, const size_t len) {
  for (size_t i = 0; i < len; i++)
    if (a[i] != b[i])
      return false;
  return true;
}
// streql for const char[n]
#define streq_lit(a, alen, b) ((sizeof(b) - 1) == (alen)) && streql(a, b, alen)
static bool streq(const char *a, const char *b) { return strcmp(a, b) == 0; }
// assumes that num_str actually has that base
static unsigned int parse_pos_int(char *num_str, size_t num_str_len,
                                  unsigned int base = 10) {
  unsigned int result = 0;
  for (size_t i = 0; i < num_str_len; i++)
    result = result * base + (num_str[i] > '9' ? num_str[i] >= 'A'
                                                     ? num_str[i] - 'A' + 10
                                                     : num_str[i] - 'a' + 10
                                               : num_str[i] - '0');
  return result;
}
static const char *num_to_str(unsigned int num, unsigned short base = 10) {
  if (num == 0)
    return "0";
  size_t len = log(num) / log(base) + 1;
  char *buf = alloc_c(len);
  for (size_t i = len; i > 0; i--) {
    unsigned short curr = num % base;
    num = num / base;
    buf[i - 1] = curr < 10 ? curr + '0' : curr + 'a' - 10;
  }
  return buf;
}

size_t unnamed_acc = 0;
// incrementing base52 (a-zA-Z) number for unnamed symbols
const char *next_unnamed() {
  size_t num = unnamed_acc++;
  if (num == 0)
    return "a"; // log(0) would fail so shortcut with correct result
  size_t len = log(num) / log(52) + 1;
  char *buf = alloc_c(len);
  for (size_t i = len; i > 0; i--) {
    unsigned short curr = num % 52;
    num /= 52;
    buf[i - 1] = curr < 26 ? curr + 'a' : curr + 'A' - 26;
  }
  return buf;
}
// Unnamed symbol
#define UN next_unnamed()

_Noreturn void error(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  exit(1);
}