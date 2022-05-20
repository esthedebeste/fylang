#pragma once
#include "reader.cpp"
#include "utils.cpp"

static std::string
    identifier_string;  // [a-zA-Z][a-zA-Z0-9]* - Filled in if T_IDENTIFIER
static char char_value; // '[^']' - Filled in if T_CHAR
static std::string num_value; // Filled in if T_NUMBER
static uint num_base;         // Filled in if T_NUMBER
static bool
    num_has_dot;      // Whether num_value contains '.' - Filled in if T_NUMBER
static char num_type; // Type of number. 'd' => double, 'f' => float, 'i' =>
                      // int32, 'u' => uint32, 'b' => byte/char/uint8
static std::string string_value; // "[^"]*" - Filled in if T_STRING
static enum {
  C_STRING,
  CHAR_ARRAY
} string_type; // Type of string. 'c' => C-string, otherwise char[len]

std::string token_to_str(const int token) {
  switch (token) {
  case T_IDENTIFIER:
    return identifier_string + " (identifier)";
  default:
    if (token < 0)
      return token_strs.at((Token)token);
    else
      return std::string(1, token);
  }
}
static char last_char = ' ';
static std::string read_str(bool (*predicate)(char)) {
  std::stringstream stream;
  stream << last_char;
  while (predicate(last_char = next_char()))
    stream << last_char;
  return stream.str();
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
static inline auto xtoi(auto c) {
  return c <= '9' ? c - '0' : c <= 'F' ? c - 'A' : c - 'a';
}
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
  case 'x': {
    // parse 2 hex digits
    char first = next_char();
    char second = next_char();
    if (!isxdigit(first) || !isxdigit(second))
      error("Expected two hex digits after \\x");
    return (xtoi(first) << 4) + xtoi(second);
  }
  default:
    error((std::string) "Invalid escape '" + escape_char + "'");
  }
}

// Returns a token, or a number of the token's ASCII value.
static int next_token() {
  while (isspace(last_char))
    last_char = next_char();
  if (last_char == EOF)
    return T_EOF;
  if (isalpha(last_char) || last_char == '_') {
    identifier_string = read_str(&is_alphaish);
    if (Token keyword = keywords[identifier_string])
      return keyword;
    return T_IDENTIFIER;
  } else if (isdigit(last_char)) {
    // Number: [0-9]+.?[0-9]*
    num_has_dot = false;
    bool started_with_zero = last_char == '0';
    std::stringstream stream;
    stream << last_char;
    last_char = next_char();
    num_base = 10;
    if (started_with_zero) {
      if (last_char == 'x')
        num_base = 16;
      else if (last_char == 'b')
        num_base = 2;
      else if (last_char == 'o')
        num_base = 8;
      else
        goto num_while;
      last_char = next_char();
    }
  num_while:
    while (true) {
      if (num_base == 16   ? !isxdigit(last_char)
          : num_base == 10 ? !isdigit(last_char)
          : num_base == 8  ? last_char < '0' || last_char > '7'
                           : last_char < '0' || last_char > '1')
        break;
      stream << last_char;
      last_char = next_char();
    }
    num_value = stream.str();
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
    std::stringstream stream;
    while ((last_char = next_char()) != '"') {
      if (last_char == EOF)
        error("Unexpected EOF in string");
      if (last_char == '\\')
        stream << get_escape(next_char());
      else
        stream << last_char;
    }
    string_value = stream.str();
    last_char = next_char();
    if (last_char == 'c') {
      string_type = C_STRING;
      last_char = next_char();
    } else
      string_type = CHAR_ARRAY;
    return T_STRING;
  } else if (last_char == '\'') {
    // Char: '[^']'
    last_char = next_char(); // eat '
    if (last_char == EOF || last_char == '\n' || last_char == '\r')
      error("Unterminated char");
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
#define eq_case(ch, token)                                                     \
  case ch:                                                                     \
    last_char = next_char();                                                   \
    return token

  if (last_char == '=') // ==, <=, >=, !=
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
    }
  if (curr_char == last_char)
    switch (curr_char) {
      eq_case('=', T_EQEQ);
      eq_case('|', T_LOR);
      eq_case('&', T_LAND);
      eq_case('<', T_LSHIFT);
      eq_case('>', T_RSHIFT);
    }
#undef eq_case
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