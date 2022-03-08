#pragma once
#include "reader.cpp"
#include "utils.cpp"

static unsigned int identifier_string_length;
static char
    *identifier_string; // [a-zA-Z][a-zA-Z0-9]* - Filled in if T_IDENTIFIER
static char char_value; // '[^']' - Filled in if T_CHAR
static char *num_value; // Filled in if T_NUMBER
static unsigned int num_length; // len(num_value) - Filled in if T_NUMBER
static bool
    num_has_dot;      // Whether num_value contains '.' - Filled in if T_NUMBER
static char num_type; // Type of number. 'd' => double, 'f' => float, 'i' =>
                      // int32, 'u' => uint32, 'b' => byte/char/uint8
static char *string_value;         // "[^"]*" - Filled in if T_STRING
static unsigned int string_length; // len(string_value) - Filled in if T_STRING

static int last_char = ' ';
static void read_str(bool (*predicate)(char), char **output,
                     unsigned int *length) {
  unsigned int curr_size = 512;
  char *str = alloc_c(curr_size);
  static unsigned int str_len = 0;
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
      error("number can't have multiple .s");
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
    fprintf(stderr, "Invalid escape '\\%c'", escape_char);
    return '\0';
  }
}

// Returns a token, or a number of the token's ASCII value.
static int next_token() {
  if (last_char == EOF)
    return T_EOF;
  while (isspace(last_char)) {
    last_char = next_char();
  }
  if (isalpha(last_char)) {
    read_str(&is_alphaish, &identifier_string, &identifier_string_length);
    if (streql(identifier_string, identifier_string_length, "fun", 3))
      return T_FUNCTION;
    else if (streql(identifier_string, identifier_string_length, "declare", 7))
      return T_DECLARE;
    else if (streql(identifier_string, identifier_string_length, "if", 2))
      return T_IF;
    else if (streql(identifier_string, identifier_string_length, "else", 4))
      return T_ELSE;
    else if (streql(identifier_string, identifier_string_length, "let", 3))
      return T_LET;
    else if (streql(identifier_string, identifier_string_length, "const", 5))
      return T_CONST;
    else if (streql(identifier_string, identifier_string_length, "while", 5))
      return T_WHILE;
    else if (streql(identifier_string, identifier_string_length, "struct", 6))
      return T_STRUCT;
    else if (streql(identifier_string, identifier_string_length, "new", 3))
      return T_NEW;
    else if (streql(identifier_string, identifier_string_length, "include", 7))
      return T_INCLUDE;
    else if (streql(identifier_string, identifier_string_length, "type", 4))
      return T_TYPE;
    else if (streql(identifier_string, identifier_string_length, "unsigned", 8))
      return T_UNSIGNED;
    else if (streql(identifier_string, identifier_string_length, "signed", 6))
      return T_SIGNED;
    else if (streql(identifier_string, identifier_string_length, "as", 2))
      return T_AS;
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
      // if not floating-point, default to long (int64)
      else
        num_type = 'l';
    }
    return T_NUMBER;
  } else if (last_char == '"') {
    // String: "[^"]*"
    unsigned int curr_size = 512;
    char *str = alloc_c(curr_size);
    unsigned int str_len = 0;
    while ((last_char = next_char()) != '"') {
      if (last_char == EOF)
        error("Unexpected EOF in string");
      if (str_len > curr_size) {
        curr_size *= 2;
        str = realloc_c(str, curr_size);
      }
      if (last_char == '\\')
        str[str_len] = get_escape(next_char());
      else
        str[str_len] = last_char;
      str_len++;
    }
    str[str_len] = '\0';
    str_len++;
    str = realloc_c(str, str_len + 1);
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
    switch (curr_char) {
    case '=': // ==
      last_char = next_char();
      return T_EQEQ;
    case '<': // <=
      last_char = next_char();
      return T_LEQ;
    case '>': // >=
      last_char = next_char();
      return T_GEQ;
    case '!': // !=
      last_char = next_char();
      return T_NEQ;
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