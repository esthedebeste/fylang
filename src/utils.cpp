#pragma once
#include "consts.cpp"
// Shortcut for '(Elem*) calloc(amount, sizeof(Elem))'
template <typename Elem> inline Elem *alloc_arr(unsigned int amount) {
  return (Elem *)calloc(amount, sizeof(Elem));
}
// Shortcut for '(Elem*) reallocarray(ptr, amount, sizeof(Elem))'
template <typename Elem>
inline Elem *realloc_arr(Elem *ptr, unsigned int amount) {
  return (Elem *)reallocarray(ptr, amount, sizeof(Elem));
}
inline char *alloc_c(unsigned int amount) { return alloc_arr<char>(amount); }
inline char *realloc_c(char *ptr, unsigned int amount) {
  return realloc_arr<char>(ptr, amount);
}

static bool streql(const char *a, const unsigned int alen, const char *b,
                   const unsigned int blen) {
  if (alen != blen)
    return false;
  for (unsigned int i = 0; i < alen; i++)
    if (a[i] != b[i])
      return false;
  return true;
}
static bool streq(const char *a, const char *b) { return strcmp(a, b) == 0; }
// assumes that num_str actually has that base
static unsigned int parse_pos_int(char *num_str, unsigned int num_str_len,
                                  unsigned int base = 10) {
  unsigned int result = 0;
  for (unsigned int i = 0; i < num_str_len; i++)
    result = result * base + (num_str[i] > '9' ? num_str[i] >= 'A'
                                                     ? num_str[i] - 'A' + 10
                                                     : num_str[i] - 'a' + 10
                                               : num_str[i] - '0');
  return result;
}
static char *num_to_str(unsigned int num, unsigned int base = 10) {
  unsigned int len = log(num) / log(base) + 1;
  char *buf = alloc_c(len);
  for (unsigned int i = len; i > 0; i--) {
    unsigned int curr = num % base;
    num = num / base;
    buf[i - 1] = curr > 9 ? curr + 'A' - 10 : curr + '0';
  }
  return buf;
}

_Noreturn void error(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  exit(1);
}