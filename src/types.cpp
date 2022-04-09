#pragma once
#include "utils.cpp"

static LLVMTypeRef void_type = LLVMStructType(nullptr, 0, true);

enum TypeType : int { Void, Number, Pointer, Function, Array, Struct, Tuple };
static std::string tt_to_str(TypeType tt) {
  switch (tt) {
  case Void:
    return "void";
  case Number:
    return "number";
  case Pointer:
    return "pointer";
  case Function:
    return "function";
  case Array:
    return "array";
  case Struct:
    return "struct";
  case Tuple:
    return "tuple";
  }
};
class PointerType;
/// Base type class.
class Type {
public:
  virtual ~Type() {}
  virtual LLVMTypeRef llvm_type() = 0;
  virtual TypeType type_type() = 0;
  virtual bool eq(Type *other) = 0;
  virtual bool neq(Type *other) { return !eq(other); }
  virtual std::string stringify() { error("stringify not implemented yet"); }
  bool operator==(Type *other) { return eq(other); }
  bool operator!=(Type *other) { return neq(other); }
  PointerType *ptr();
  virtual size_t _hash() = 0;
};
template <> struct std::hash<Type *> {
  std::size_t operator()(Type *const &s) const noexcept { return s->_hash(); }
};
template <typename T> inline size_t hash(T t) { return std::hash<T>()(t); }
template <> struct std::hash<std::vector<Type *>> {
  size_t operator()(const std::vector<Type *> type) const {
    size_t res = 0;
    for (auto t : type)
      res = (res << 5) + t->_hash();
    return res ^ std::hash<size_t>()(type.size());
  }
};

class VoidType : public Type {
public:
  VoidType() {}
  LLVMTypeRef llvm_type() { return void_type; }
  TypeType type_type() { return TypeType::Void; }
  bool eq(Type *other) { return other->type_type() == TypeType::Void; }
  std::string stringify() { return "void"; }
  size_t _hash() { return hash(TypeType::Void); }
};

class NumType : public Type {
public:
  unsigned int byte_size;
  unsigned int bits;
  bool is_floating;
  bool is_signed;
  NumType(unsigned int bits, bool is_floating, bool is_signed)
      : bits(bits), is_floating(is_floating), is_signed(is_signed) {
    byte_size = bits / 8;
  }
  NumType(std::string bits_str, bool is_floating, bool is_signed)
      : is_floating(is_floating), is_signed(is_signed) {
    if (bits_str == "_ptrsize")
      bits = LLVMPointerSize(target_data) * 8;
    else
      bits = std::stoi(bits_str);
    byte_size = bits / 8;
  }
  // Pointer-size
  NumType(bool is_signed) : is_floating(false), is_signed(is_signed) {
    byte_size = LLVMPointerSize(target_data);
    bits = byte_size * 8;
  }
  LLVMTypeRef llvm_type() {
    if (!is_floating)
      return LLVMIntType(bits);
    switch (bits) {
    case 16:
      return LLVMHalfType();
    case 32:
      return LLVMFloatType();
    case 64:
      return LLVMDoubleType();
    case 128:
      return LLVMFP128Type();
    default:
      error("floating " + std::to_string(bits) + "-bit numbers don't exist");
    }
  }
  TypeType type_type() { return TypeType::Number; }
  bool eq(Type *other) {
    if (NumType *other_n = dynamic_cast<NumType *>(other))
      return other_n->bits == bits && other_n->is_floating == is_floating &&
             other_n->is_signed == is_signed;
    return false;
  }
  std::string stringify() {
    std::string typ = is_floating ? "float" : is_signed ? "int" : "uint";
    return typ + std::to_string(bits);
  }
  size_t _hash() {
    return hash(bits) ^ hash(is_floating) ^ hash(is_signed) ^
           hash(TypeType::Number);
  }
};
class PointerType : public Type {
public:
  Type *points_to;
  PointerType(Type *points_to) : points_to(points_to) {}
  Type *get_points_to() { return points_to; }
  LLVMTypeRef llvm_type() {
    return LLVMPointerType(this->points_to->llvm_type(), 0);
  }
  TypeType type_type() { return TypeType::Pointer; }
  bool eq(Type *other) {
    if (other->type_type() == TypeType::Function)
      return other->eq(this);
    if (PointerType *other_n = dynamic_cast<PointerType *>(other))
      return other_n->points_to->eq(this->points_to);
    return false;
  }

  std::string stringify() { return "*" + points_to->stringify(); }
  size_t _hash() { return hash(points_to) ^ hash(TypeType::Pointer); }
};
PointerType *Type::ptr() { return new PointerType(this); }
class ArrayType : public Type {
public:
  Type *elem;
  unsigned int count;
  ArrayType(Type *elem, unsigned int count) : elem(elem), count(count) {}
  Type *get_elem_type() { return elem; }
  unsigned int get_elem_count() { return count; }
  LLVMTypeRef llvm_type() { return LLVMArrayType(elem->llvm_type(), count); }
  TypeType type_type() { return TypeType::Array; }
  bool eq(Type *other) {
    if (ArrayType *other_arr = dynamic_cast<ArrayType *>(other))
      return other_arr->elem->eq(this->elem) && other_arr->count == this->count;
    return false;
  }

  std::string stringify() {
    return elem->stringify() + "[" + std::to_string(count) + "]";
  }
  size_t _hash() { return hash(elem) ^ hash(count) ^ hash(TypeType::Array); }
};
class TupleType : public Type {
public:
  LLVMTypeRef llvm_struct_type;
  std::vector<Type *> types;
  TupleType(std::vector<Type *> types) : types(types) {
    LLVMTypeRef *llvm_types = new LLVMTypeRef[types.size()];
    for (size_t i = 0; i < types.size(); i++)
      llvm_types[i] = types[i]->llvm_type();
    llvm_struct_type = LLVMStructType(llvm_types, types.size(), true);
  }
  Type *get_elem_type(size_t index) {
    if (index >= types.size())
      error("Struct get_elem_type index out of bounds");
    return types[index];
  }
  LLVMTypeRef llvm_type() { return llvm_struct_type; }
  TypeType type_type() { return TypeType::Tuple; }
  bool eq(Type *other) {
    if (TupleType *other_s = dynamic_cast<TupleType *>(other)) {
      if (other_s->types.size() == 0)
        return true; // empty tuple is equal to any other tuple, for `unknown`
      if (other_s->types.size() != types.size())
        return false;
      for (size_t i = 0; i < types.size(); i++)
        if (other_s->types[i]->neq(types[i]))
          return false;
      return true;
    }
    return false;
  }
  std::string stringify() {
    std::stringstream res;
    res << "{ ";
    for (size_t i = 0; i < types.size(); i++) {
      if (i != 0)
        res << ", ";
      res << types[i]->stringify();
    }
    res << " }";
    return res.str();
  }
  size_t _hash() {
    return hash(types) ^ hash(types.size()) ^ hash(TypeType::Tuple);
  }
};
class StructType : public TupleType {
public:
  std::string name;
  std::vector<std::pair<std::string, Type *>> fields;
  StructType(std::string name,
             std::vector<std::pair<std::string, Type *>> fields)
      : name(name), fields(fields), TupleType(seconds(fields)) {
    LLVMTypeRef *llvm_types = new LLVMTypeRef[fields.size()];
    for (size_t i = 0; i < fields.size(); i++)
      llvm_types[i] = types[i]->llvm_type();
    llvm_struct_type = LLVMStructCreateNamed(curr_ctx, name.c_str());
    LLVMStructSetBody(llvm_struct_type, llvm_types, fields.size(), true);
  }
  size_t get_index(std::string name) {
    for (size_t i = 0; i < fields.size(); i++)
      if (fields[i].first == name)
        return i;
    error("Struct does not have key " + name);
  }
  LLVMTypeRef llvm_type() { return llvm_struct_type; }
  TypeType type_type() { return TypeType::Struct; }
  bool eq(Type *other) {
    if (StructType *other_s = dynamic_cast<StructType *>(other))
      // structs are unique
      return this == other_s;
    return false;
  }
  std::string stringify() { return name; }
  size_t _hash() {
    size_t h = 0;
    for (auto f : fields)
      h ^= hash(f.first) ^ hash(f.second);
    return h ^ hash(fields.size()) ^ hash(TypeType::Struct);
  }
};
class FunctionType : public Type {
public:
  Type *return_type;
  std::vector<Type *> arguments;
  bool vararg;

  FunctionType(Type *return_type, std::vector<Type *> arguments, bool vararg)
      : return_type(return_type), arguments(arguments), vararg(vararg) {}
  LLVMTypeRef llvm_type() {
    LLVMTypeRef *llvm_args = new LLVMTypeRef[arguments.size()];
    for (size_t i = 0; i < arguments.size(); i++)
      llvm_args[i] = arguments[i]->llvm_type();
    return LLVMFunctionType(return_type->llvm_type(), llvm_args,
                            arguments.size(), vararg);
  }
  TypeType type_type() { return TypeType::Function; }
  bool eq(Type *other) {
    if (PointerType *ptr = dynamic_cast<PointerType *>(other))
      return this->eq(ptr->get_points_to());
    FunctionType *other_f = dynamic_cast<FunctionType *>(other);
    if (!other_f || other_f->arguments.size() != arguments.size() ||
        other_f->vararg != vararg || other_f->return_type->neq(return_type))
      return false;
    for (unsigned int i = 0; i < arguments.size(); i++)
      if (other_f->arguments[i]->neq(arguments[i]))
        return false;
    return true;
  };
  std::string stringify() {
    std::stringstream res;
    res << "fun(";
    for (size_t i = 0; i < arguments.size(); i++) {
      if (i != 0)
        res << ", ";
      res << arguments[i]->stringify();
    }
    res << "): " << return_type->stringify();
    return res.str();
  }
  size_t _hash() {
    return hash(arguments) ^ hash(arguments.size()) ^ hash(TypeType::Function);
  }
};