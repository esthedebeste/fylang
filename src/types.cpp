#pragma once
#include "utils.cpp"

static LLVMTypeRef void_type =
    LLVMStructCreateNamed(LLVMGetGlobalContext(), "void");

enum TypeType { Void, Number, Pointer, Function, Array, Struct, Tuple };
static const char *tt_to_str(TypeType tt) {
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
/// Base type class.
class Type {
public:
  virtual ~Type() {}
  virtual LLVMTypeRef llvm_type() = 0;
  virtual TypeType type_type() = 0;
  virtual bool eq(Type *other) {
    return this->type_type() == other->type_type();
  };
  virtual bool neq(Type *other) { return !eq(other); }
  virtual const char *stringify() { error("stringify not implemented yet"); }
};

class VoidType : public Type {
public:
  VoidType() {}
  LLVMTypeRef llvm_type() { return void_type; }
  TypeType type_type() { return TypeType::Void; }
  const char *stringify() { return "void"; }
};

class NumType : public Type {
public:
  unsigned int byte_size;
  unsigned int bits;
  bool is_floating;
  bool is_signed;
  NumType(unsigned int bits, bool is_floating, bool is_signed)
      : bits(bits), is_floating(is_floating), is_signed(is_signed) {
    byte_size = bits * 8;
  }
  NumType(char *bits_str, size_t bits_str_len, bool is_floating, bool is_signed)
      : is_floating(is_floating), is_signed(is_signed) {
    if (streq_lit(bits_str, bits_str_len, "_ptrsize"))
      bits = LLVMPointerSize(target_data) * 8;
    else
      bits = parse_pos_int(bits_str, bits_str_len, 10);
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
      error("floating %d-bit numbers don't exist", bits);
    }
  }
  TypeType type_type() { return TypeType::Number; }
  bool eq(Type *other) {
    if (NumType *other_n = dynamic_cast<NumType *>(other))
      return other_n->bits == bits && other_n->is_floating == is_floating &&
             other_n->is_signed == is_signed;
    return false;
  }
  const char *stringify() {
    const char *typ = is_floating ? "float" : is_signed ? "int" : "uint";
    char *str = alloc_c(strlen(typ) + log10(bits));
    strcat(str, typ);
    strcat(str, num_to_str(bits));
    return str;
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

  const char *stringify() {
    const char *contains = points_to->stringify();
    char *str = alloc_c(1 /* '*' */ + strlen(contains));
    str[0] = '*';
    strcat(str + 1, contains);
    return str;
  }
};
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

  const char *stringify() {
    const char *contains = elem->stringify();
    size_t count_len = log10(count);
    char *str = alloc_c(2 + strlen(contains) + 3 + count_len + 2);
    strcat(str, "( ");
    strcat(str, contains);
    strcat(str, " * ");
    strcat(str, num_to_str(count));
    strcat(str, " )");
    return str;
  }
};
class TupleType : public Type {
public:
  LLVMTypeRef llvm_struct_type;
  Type **types;
  size_t length;
  TupleType(Type **types, size_t length) : types(types), length(length) {
    LLVMTypeRef *llvm_types = alloc_arr<LLVMTypeRef>(length);
    for (size_t i = 0; i < length; i++)
      llvm_types[i] = types[i]->llvm_type();
    llvm_struct_type = LLVMStructType(llvm_types, length, true);
  }
  Type *get_elem_type(size_t index) {
    if (index >= length)
      error("Struct get_elem_type index out of bounds");
    return types[index];
  }
  LLVMTypeRef llvm_type() { return llvm_struct_type; }
  TypeType type_type() { return TypeType::Tuple; }
  bool eq(Type *other) {
    if (TupleType *other_s = dynamic_cast<TupleType *>(other)) {
      if (other_s->length == 0)
        return true; // empty tuple is equal to any other tuple, for `unknown`
      if (other_s->length != length)
        return false;
      for (size_t i = 0; i < length; i++)
        if (other_s->types[i]->neq(types[i]))
          return false;
      return true;
    }
    return false;
  }
  const char *stringify() {
    size_t len = 4;
    size_t *lens = alloc_arr<size_t>(length);
    const char **strs = alloc_arr<const char *>(length);
    for (size_t i = 0; i < length; i++) {
      strs[i] = types[i]->stringify();
      len += lens[i] = strlen(strs[i]);
    }
    len += length * 2;
    char *buf = alloc_arr<char>(len);
    strcpy(buf, "{ ");
    for (size_t i = 0; i < length; i++) {
      strncat(buf, strs[i], lens[i]);
      strncat(buf, ", ", 2);
    }
    strncat(buf, " }", 2);
    return buf;
  }
};
class StructType : public TupleType {
public:
  char *name;
  size_t name_len;
  char **keys;
  size_t *key_lengths;
  StructType(char *name, size_t name_len, char **keys, size_t *key_lengths,
             Type **types, size_t length)
      : TupleType(types, length), name(name), name_len(name_len), keys(keys),
        key_lengths(key_lengths) {
    LLVMTypeRef *llvm_types = alloc_arr<LLVMTypeRef>(length);
    for (size_t i = 0; i < length; i++)
      llvm_types[i] = types[i]->llvm_type();
    llvm_struct_type = LLVMStructCreateNamed(curr_ctx, name);
    LLVMStructSetBody(llvm_struct_type, llvm_types, length, true);
  }
  size_t get_index(char *name, size_t name_len) {
    for (size_t i = 0; i < length; i++) {
      if (key_lengths[i] == name_len) {
        for (size_t j = 0; j < name_len; j++)
          if (keys[i][j] != name[j])
            continue;
        return i;
      }
    }
    error("Struct does not have key");
  }
  LLVMTypeRef llvm_type() { return llvm_struct_type; }
  TypeType type_type() { return TypeType::Struct; }
  bool eq(Type *other) {
    if (StructType *other_s = dynamic_cast<StructType *>(other))
      // structs are unique
      return this == other_s;
    return false;
  }
  const char *stringify() { return name; }
};
class FunctionType : public Type {
public:
  Type *return_type;
  Type **arguments;
  unsigned int arg_count;
  bool vararg;

  FunctionType(Type *return_type, Type **arguments, unsigned int arg_count,
               bool vararg)
      : return_type(return_type), arguments(arguments), arg_count(arg_count),
        vararg(vararg) {}
  LLVMTypeRef llvm_type() {
    LLVMTypeRef *llvm_args = alloc_arr<LLVMTypeRef>(arg_count);
    for (unsigned int i = 0; i < arg_count; i++)
      llvm_args[i] = arguments[i]->llvm_type();
    return LLVMFunctionType(return_type->llvm_type(), llvm_args, arg_count,
                            vararg);
  }
  TypeType type_type() { return TypeType::Function; }
  bool eq(Type *other) {
    if (PointerType *ptr = dynamic_cast<PointerType *>(other))
      return this->eq(ptr->get_points_to());
    FunctionType *other_f = dynamic_cast<FunctionType *>(other);
    if (!other_f)
      return false;
    if (other_f->arg_count != arg_count)
      return false;
    if (other_f->vararg != vararg)
      return false;
    if (other_f->return_type->neq(return_type))
      return false;
    for (unsigned int i = 0; i < arg_count; i++)
      if (other_f->arguments[i]->neq(arguments[i]))
        return false;
    return true;
  };
  const char *stringify() {
    const char **arg_strs = alloc_arr<const char *>(arg_count);
    size_t final_len = 7; /* "fun(): " */
    for (unsigned int i = 0; i < arg_count; i++) {
      const char *type = arguments[i]->stringify();
      arg_strs[i] = type;
      final_len += strlen(type) + 2 /*", "*/;
    }
    const char *ret_str = return_type->stringify();
    final_len += strlen(ret_str);
    char *str = alloc_c(final_len);
    strcat(str, "fun(");
    for (unsigned int i = 0; i < arg_count; i++) {
      strcat(str, arg_strs[i]);
      strcat(str, ", ");
    }
    strcat(str, "): ");
    strcat(str, ret_str);
    return str;
  }
};