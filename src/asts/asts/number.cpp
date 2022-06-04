#include "../asts.h"

std::unordered_map<char, NumType> num_type_chars = {
    {'b', NumType(8, false, false)}, {'d', NumType(64, true, true)},
    {'f', NumType(32, true, true)},  {'i', NumType(32, false, true)},
    {'l', NumType(64, false, true)}, {'u', NumType(32, false, false)},
};
NumType num_char_to_type(char type_char, bool has_dot) {
  if (num_type_chars.count(type_char) == 0)
    error((std::string) "Invalid number type id '" + type_char + "'/"
          << (int)type_char);
  auto &num_type = num_type_chars[type_char];
  if (has_dot && !num_type.is_floating)
    error((std::string) "'" + type_char + "' (" + num_type.stringify() +
          ") type can't have decimals");
  return num_type;
}

NumberExprAST::NumberExprAST(std::string val, char type_char, bool has_dot,
                             unsigned int base)
    : type(num_char_to_type(type_char, has_dot)) {
  if (type.is_floating)
    if (base != 10)
      error("floating-point numbers with a base that isn't decimal aren't "
            "supported.");
    else
      value.floating = std::stold(val);
  else
    value.integer = std::stoull(val, nullptr, base);
}
NumberExprAST::NumberExprAST(unsigned long long val, char type_char)
    : type(num_char_to_type(type_char, false)) {
  value.integer = val;
}
NumberExprAST::NumberExprAST(unsigned long long val, NumType type)
    : type(type) {
  value.integer = val;
}
Type *NumberExprAST::get_type() { return &type; }
Value *NumberExprAST::gen_value() {
  if (type.is_floating)
    return new ConstValue(&type,
                          LLVMConstReal(type.llvm_type(), value.floating));
  else
    return new IntValue(type, value.integer);
}
bool NumberExprAST::is_constant() { return true; }