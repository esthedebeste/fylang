#pragma once
#include "reader.h"
#include "utils.h"

extern std::string
    identifier_string;  // [a-zA-Z][a-zA-Z0-9]* - Filled in if T_IDENTIFIER
extern char char_value; // '[^']' - Filled in if T_CHAR
extern std::string num_value; // Filled in if T_NUMBER
extern uint num_base;         // Filled in if T_NUMBER
extern bool
    num_has_dot;      // Whether num_value contains '.' - Filled in if T_NUMBER
extern char num_type; // Type of number. 'd' => double, 'f' => float, 'i' =>
                      // int32, 'u' => uint32, 'b' => byte/char/uint8
extern std::string string_value; // "[^"]*" - Filled in if T_STRING
enum StringType { C_STRING, CHAR_ARRAY, PTR_CHAR_ARRAY };
extern StringType string_type; // Type of string

std::string token_to_str(const int token);
extern char last_char;
std::string read_str(bool (*predicate)(char));
bool is_numish(char c);
bool isnt_quot(char c);
bool isnt_space(char c);
bool is_alphaish(char c);
char get_escape(char escape_char);
int next_token();