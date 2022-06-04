#pragma once
#include "utils.h"

enum TypeType : int {
  Null,
  Number,
  Pointer,
  Function,
  Array,
  Struct,
  Tuple,
};
class PointerType;
class FunctionAST;
/// Base type class.
class Type {
public:
  virtual ~Type();
  virtual LLVMTypeRef llvm_type() = 0;
  virtual TypeType type_type() = 0;
  virtual bool eq(Type *other) = 0;
  virtual bool neq(Type *other);
  virtual bool castable_to(Type *other);
  virtual std::string stringify();
  virtual FunctionAST *get_destructor(); // defined in asts/functions.cpp
  bool operator==(Type *other);
  bool operator!=(Type *other);
  PointerType *ptr();
  virtual size_t _hash() = 0;
};
template <> struct std::hash<Type *> {
  size_t operator()(Type *const &s) const noexcept { return s->_hash(); }
};
template <> struct std::hash<std::vector<Type *>> {
  size_t operator()(const std::vector<Type *> type) const {
    size_t res = 0;
    for (auto t : type)
      res = (res << 4) + t->_hash();
    return res ^ std::hash<size_t>()(type.size());
  }
};

class NullType : public Type {
public:
  NullType();
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
extern NullType null_type;

class NumType : public Type {
public:
  unsigned int byte_size;
  unsigned int bits;
  bool is_floating;
  bool is_signed;
  NumType(unsigned int bits, bool is_floating, bool is_signed);
  NumType(std::string bits_str, bool is_floating, bool is_signed);
  // Pointer-size
  explicit NumType(bool is_signed);
  // exists for maps, do not use.
  explicit NumType();
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
class PointerType : public Type {
public:
  Type *points_to;
  PointerType(Type *points_to);
  Type *get_points_to();
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
class ArrayType : public Type {
public:
  Type *elem;
  uint count;
  ArrayType(Type *elem, uint count);
  Type *get_elem_type();
  uint get_elem_count();
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
class TupleType : public Type {
public:
  LLVMTypeRef llvm_struct_type;
  std::vector<Type *> types;
  TupleType(std::vector<Type *> types);
  Type *get_elem_type(size_t index);
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
class StructType : public TupleType {
public:
  std::vector<std::pair<std::string, Type *>> fields;
  StructType(std::vector<std::pair<std::string, Type *>> fields);
  size_t get_index(std::string name);
  TypeType type_type();
  size_t _hash();
  bool eq(Type *other);
  std::string stringify();
};
class NamedStructType : public StructType {
public:
  std::string name;
  NamedStructType(std::string name,
                  std::vector<std::pair<std::string, Type *>> fields);
  bool gave_name = false;
  LLVMTypeRef llvm_type();
  bool eq(Type *other);
  std::string stringify();
};
class FunctionType : public Type {
public:
  Type *return_type;
  std::vector<Type *> arguments;
  FuncFlags flags;

  FunctionType(Type *return_type, std::vector<Type *> arguments,
               FuncFlags flags);
  LLVMTypeRef llvm_type();
  TypeType type_type();
  bool eq(Type *other);
  bool castable_to(Type *other);
  std::string stringify();
  size_t _hash();
};
template <> struct std::hash<FunctionType *> {
  size_t operator()(FunctionType *t) const noexcept { return t->_hash(); }
};

template <> struct std::equal_to<FunctionType *> {
  bool operator()(FunctionType *const &a, FunctionType *const &b) const {
    return a->eq(b);
  }
};