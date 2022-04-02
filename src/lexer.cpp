#pragma once
#include "reader.cpp"
#include "utils.cpp"

static size_t identifier_string_length;
static char
    *identifier_string;   // [a-zA-Z][a-zA-Z0-9]* - Filled in if T_IDENTIFIER
static char char_value;   // '[^']' - Filled in if T_CHAR
static char *num_value;   // Filled in if T_NUMBER
static size_t num_length; // len(num_value) - Filled in if T_NUMBER
static bool
    num_has_dot;      // Whether num_value contains '.' - Filled in if T_NUMBER
static char num_type; // Type of number. 'd' => double, 'f' => float, 'i' =>
                      // int32, 'u' => uint32, 'b' => byte/char/uint8
static char *string_value;   // "[^"]*" - Filled in if T_STRING
static size_t string_length; // len(string_value) - Filled in if T_STRING

static int last_char = ' ';
static void read_str(bool (*predicate)(char), char **output, size_t *length) {
  size_t curr_size = 512;
  char *str = alloc_c(curr_size);
  static size_t str_len = 0;
  str[0] = last_char;
  str_len = 1;
  while (predicate(last_char = next_char())) {
    if (str_len > curr_size) {
      curr_size *= 2;
      str = realloc_c(str, curr_size);
    }
    str[str_len] = last_char;
    str_len++;
  }
  str[str_len] = '\0';
  str = realloc_c(str, str_len + 1);
  *output = str;
  *length = str_len;
  return;
}
// isdigit(c) || c=='.'
static bool is_numish(char c) {
  if (c == '.') {
    if (num_has_dot)
      return false;
    num_has_dot = true;
    return true;
  }
  return isdigit(c);
}
// c != '"'
static bool isnt_quot(char c) { return c != '"'; }
// (!isspace(c))
static bool isnt_space(char c) { return !isspace(c); }
static bool is_alphaish(char c) { return isalpha(c) || isdigit(c) || c == '_'; }
static char get_escape(char escape_char) {
  switch (escape_char) {
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case '\'':
    return '\'';
  case '"':
    return '"';
  case '\\':
    return '\\';
  case '0':
    return '\0';
  default:
    error("Invalid escape '\\%c'", escape_char);
  }
}

// Returns a token, or a number of the token's ASCII value.
static int next_token() {
  while (isspace(last_char))
    last_char = next_char();
  if (last_char == EOF)
    return T_EOF;
  if (isalpha(last_char) || last_char == '_') {
    read_str(&is_alphaish, &identifier_string, &identifier_string_length);
#define T_eq(str) streq_lit(identifier_string, identifier_string_length, str)
#define token(str, t)                                                          \
  if (T_eq(str))                                                               \
  return t
#define etoken(str, t) else if (T_eq(str)) return t
    token("fun", T_FUNCTION);
    etoken("declare", T_DECLARE);
    etoken("if", T_IF);
    etoken("else", T_ELSE);
    etoken("let", T_LET);
    etoken("const", T_CONST);
    etoken("while", T_WHILE);
    etoken("struct", T_STRUCT);
    etoken("new", T_NEW);
    etoken("include", T_INCLUDE);
    etoken("type", T_TYPE);
    etoken("unsigned", T_UNSIGNED);
    etoken("signed", T_SIGNED);
    etoken("as", T_AS);
    // __ because vararg can't be used outside of declarations
    etoken("__VARARG__", T_VARARG);
    etoken("typeof", T_TYPEOF);
    etoken("true", T_TRUE);
    etoken("false", T_FALSE);
    etoken("return", T_RETURN);
    etoken("for", T_FOR);
    etoken("DUMP", T_DUMP);
    etoken("sizeof", T_SIZEOF);
#undef etoken
#undef token
#undef T_eq
    return T_IDENTIFIER;
  } else if (isdigit(last_char)) {
    // Number: [0-9]+.?[0-9]*
    num_has_dot = false;
    read_str(&is_numish, &num_value, &num_length);
    if (last_char == 'd' || last_char == 'l' || last_char == 'f' ||
        last_char == 'i' || last_char == 'u' || last_char == 'b') {
      num_type = last_char;
      last_char = next_char();
    } else {
      // if floating-point, default to double (float64)
      if (num_has_dot)
        num_type = 'd';
      // if not floating-point, default to int (int32)
      else
        num_type = 'i';
    }
    return T_NUMBER;
  } else if (last_char == '"') {
    // String: "[^"]*"
    size_t curr_size = 512;
    char *str = alloc_c(curr_size);
    size_t str_len = 0;
    while ((last_char = next_char()) != '"') {
      if (last_char == EOF)
        error("Unexpected EOF in string");
      if (str_len > curr_size - 1) { // leave room for null-byte
        curr_size *= 2;
        str = realloc_c(str, curr_size);
      }
      if (last_char == '\\')
        str[str_len] = get_escape(next_char());
      else
        str[str_len] = last_char;
      str_len++;
    }
    str[str_len++] = '\0';
    string_value = str;
    string_length = str_len;
    last_char = next_char();
    return T_STRING;
  } else if (last_char == '\'') {
    // Char: '[^']'
    last_char = next_char(); // eat '
    if (last_char == EOF || last_char == '\n' || last_char == '\r')
      error("Unterminated char\n");
    if (last_char == '\\')
      char_value = get_escape(next_char());
    else
      char_value = last_char;
    if (next_char() != '\'') // eat '
      error("char with length above 1");
    last_char = next_char();
    return T_CHAR;
  }

  int curr_char = last_char;
  last_char = next_char();
  if (last_char == '=') // ==, <=, >=, !=
#define eq_case(ch, token)                                                     \
  case ch:                                                                     \
    last_char = next_char();                                                   \
    return token

    switch (curr_char) {
      eq_case('=', T_EQEQ);
      eq_case('<', T_LEQ);
      eq_case('>', T_GEQ);
      eq_case('!', T_NEQ);
      eq_case('+', T_PLUSEQ);
      eq_case('-', T_MINEQ);
      eq_case('*', T_STAREQ);
      eq_case('/', T_SLASHEQ);
      eq_case('%', T_PERCENTEQ);
      eq_case('&', T_ANDEQ);
      eq_case('|', T_OREQ);
#undef eq_case
    }

  if (curr_char == '|' && last_char == '|') // ||
  {
    last_char = next_char();
    return T_LOR;
  }
  if (curr_char == '&' && last_char == '&') // &&
  {
    last_char = next_char();
    return T_LAND;
  }
  if (curr_char == '/') {
    if (last_char == '/') {
      // Comment: //[^\n\r]*
      do
        last_char = next_char();
      while (last_char != EOF && last_char != '\n' && last_char != '\r');
      return next_token(); // could recurse overflow, might make a wrapper
                           // function that while's and a T_COMMENT type
    } else if (last_char == '*') {
      // Comment: /* .* */
      char last;
      do {
        last = last_char;
        last_char = next_char();
      } while (last_char != EOF && !(last == '*' && last_char == '/'));
      last_char = next_char();
    } else
      return curr_char;
    return next_token(); // could recurse overflow, might make a wrapper
                         // function that while's and a T_COMMENT type
  }

  // Otherwise, just return the character as its ascii value.
  return curr_char;
}