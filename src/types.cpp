#include "types.h"
#include "utils.h"

Type::~Type() {}
bool Type::neq(Type *other) { return !eq(other); }
bool Type::castable_to(Type *other) { return eq(other); }
std::string Type::stringify() { error("stringify not implemented yet"); }
bool Type::operator==(Type *other) { return eq(other); }
bool Type::operator!=(Type *other) { return neq(other); }
PointerType *Type::ptr() { return new PointerType(this); }
template <typename T> inline size_t hash(T t) { return std::hash<T>()(t); }

NullType null_type = NullType();
NullType::NullType() {}
LLVMTypeRef NullType::llvm_type() { return LLVMStructType(nullptr, 0, true); }
TypeType NullType::type_type() { return TypeType::Null; }
bool NullType::eq(Type *other) { return other->type_type() == TypeType::Null; }
bool NullType::castable_to(Type *other) { return true; }
std::string NullType::stringify() { return "null"; }
size_t NullType::_hash() { return hash(TypeType::Null); }

NumType::NumType(unsigned int bits, bool is_floating, bool is_signed)
    : bits(bits), is_floating(is_floating), is_signed(is_signed) {
  byte_size = bits / 8;
}
NumType::NumType(std::string bits_str, bool is_floating, bool is_signed)
    : is_floating(is_floating), is_signed(is_signed) {
  if (bits_str == "_ptrsize")
    bits = LLVMPointerSize(target_data) * 8;
  else
    bits = std::stoi(bits_str);
  byte_size = bits / 8;
}
NumType::NumType(bool is_signed) : is_floating(false), is_signed(is_signed) {
  byte_size = LLVMPointerSize(target_data);
  bits = byte_size * 8;
}
NumType::NumType() {}
LLVMTypeRef NumType::llvm_type() {
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
TypeType NumType::type_type() { return TypeType::Number; }
bool NumType::eq(Type *other) {
  if (NumType *other_n = dynamic_cast<NumType *>(other))
    return other_n->bits == bits && other_n->is_floating == is_floating &&
           other_n->is_signed == is_signed;
  return false;
}
bool NumType::castable_to(Type *other) {
  return other->type_type() == TypeType::Number ||
         other->type_type() == TypeType::Pointer;
}
std::string NumType::stringify() {
  std::string typ = is_floating ? "float" : is_signed ? "int" : "uint";
  return typ + std::to_string(bits);
}
size_t NumType::_hash() {
  return hash(bits) ^ hash(is_floating) ^ hash(is_signed) ^
         hash(TypeType::Number);
}

PointerType::PointerType(Type *points_to) : points_to(points_to) {}
Type *PointerType::get_points_to() { return points_to; }
LLVMTypeRef PointerType::llvm_type() {
  return LLVMPointerType(this->points_to->llvm_type(), 0);
}
TypeType PointerType::type_type() { return TypeType::Pointer; }
bool PointerType::eq(Type *other) {
  if (other->type_type() == TypeType::Function)
    return other->eq(this);
  if (PointerType *other_n = dynamic_cast<PointerType *>(other))
    return other_n->points_to->eq(this->points_to);
  return false;
}
bool PointerType::castable_to(Type *other) {
  if (other->type_type() == TypeType::Pointer ||
      other->type_type() == TypeType::Number)
    return true;
  else if (other->type_type() == TypeType::Function)
    return this->points_to->eq(other);
  else
    return false;
}
std::string PointerType::stringify() { return "*" + points_to->stringify(); }
size_t PointerType::_hash() {
  return hash(points_to) ^ hash(TypeType::Pointer);
}

ArrayType::ArrayType(Type *elem, uint count) : elem(elem), count(count) {}
Type *ArrayType::get_elem_type() { return elem; }
uint ArrayType::get_elem_count() { return count; }
LLVMTypeRef ArrayType::llvm_type() {
  return LLVMArrayType(elem->llvm_type(), count);
}
TypeType ArrayType::type_type() { return TypeType::Array; }
bool ArrayType::eq(Type *other) {
  if (ArrayType *other_arr = dynamic_cast<ArrayType *>(other))
    return other_arr->elem->eq(this->elem) && other_arr->count == this->count;
  return false;
}
bool ArrayType::castable_to(Type *other) {
  if (other->type_type() == TypeType::Array) {
    ArrayType *other_arr = dynamic_cast<ArrayType *>(other);
    return other_arr->elem->eq(this->elem) && other_arr->count >= this->count;
  } else if (PointerType *other_ptr = dynamic_cast<PointerType *>(other))
    return PointerType(this->elem).castable_to(other_ptr);
  return false;
}
std::string ArrayType::stringify() {
  return elem->stringify() + "[" + std::to_string(count) + "]";
}
size_t ArrayType::_hash() {
  return hash(elem) ^ hash(count) ^ hash(TypeType::Array);
}

TupleType::TupleType(std::vector<Type *> types) : types(types) {
  LLVMTypeRef *llvm_types = new LLVMTypeRef[types.size()];
  for (size_t i = 0; i < types.size(); i++)
    llvm_types[i] = types[i]->llvm_type();
  llvm_struct_type = LLVMStructType(llvm_types, types.size(), true);
}
Type *TupleType::get_elem_type(size_t index) {
  if (index >= types.size())
    error("Struct get_elem_type index out of bounds");
  return types[index];
}
LLVMTypeRef TupleType::llvm_type() { return llvm_struct_type; }
TypeType TupleType::type_type() { return TypeType::Tuple; }
bool TupleType::eq(Type *other) {
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
bool TupleType::castable_to(Type *other) {
  if (other->type_type() == TypeType::Tuple) {
    TupleType *other_tuple = dynamic_cast<TupleType *>(other);
    if (other_tuple->types.size() == 0)
      return true;
    if (other_tuple->types.size() != types.size())
      return false;
    for (size_t i = 0; i < types.size(); i++) {
      if (!other_tuple->types[i]->castable_to(types[i]))
        return false;
    }
    return true;
  }
  return false;
}
std::string TupleType::stringify() {
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
size_t TupleType::_hash() {
  return hash(types) ^ hash(types.size()) ^ hash(TypeType::Tuple);
}

StructType::StructType(std::vector<std::pair<std::string, Type *>> fields)
    : fields(fields), TupleType(seconds(fields)) {}
size_t StructType::get_index(std::string name) {
  for (size_t i = 0; i < fields.size(); i++)
    if (fields[i].first == name)
      return i;
  error("Struct does not have key " + name);
}
TypeType StructType::type_type() { return TypeType::Struct; }
size_t StructType::_hash() {
  size_t h = 0;
  for (auto f : fields)
    h ^= hash(f.first) ^ hash(f.second);
  return h ^ hash(fields.size()) ^ hash(TypeType::Struct);
}
bool StructType::eq(Type *other) {
  if (StructType *other_s = dynamic_cast<StructType *>(other)) {
    if (other_s->fields.size() != fields.size())
      return false;
    for (size_t i = 0; i < fields.size(); i++)
      if (other_s->fields[i].first != fields[i].first ||
          other_s->fields[i].second->neq(fields[i].second))
        return false;
    return true;
  } else if (TupleType *other_s = dynamic_cast<TupleType *>(other))
    return TupleType::eq(other);
  return false;
}
std::string StructType::stringify() {
  std::stringstream res;
  res << "{ ";
  for (size_t i = 0; i < types.size(); i++) {
    if (i != 0)
      res << ", ";
    res << fields[i].first << ": " << fields[i].second->stringify();
  }
  res << " }";
  return res.str();
}

NamedStructType::NamedStructType(
    std::string name, std::vector<std::pair<std::string, Type *>> fields)
    : name(name), StructType(fields) {}
LLVMTypeRef NamedStructType::llvm_type() {
  if (!gave_name) {
    llvm_struct_type = LLVMStructCreateNamed(curr_ctx, name.c_str());
    LLVMTypeRef *llvm_types = new LLVMTypeRef[fields.size()];
    for (size_t i = 0; i < fields.size(); i++)
      llvm_types[i] = fields[i].second->llvm_type();
    LLVMStructSetBody(llvm_struct_type, llvm_types, fields.size(), true);
    gave_name = true;
  }
  return llvm_struct_type;
}
bool NamedStructType::eq(Type *other) {
  if (NamedStructType *other_s = dynamic_cast<NamedStructType *>(other))
    if (other_s->name != name)
      return false;
  return StructType::eq(other);
}
std::string NamedStructType::stringify() { return name; }

FunctionType::FunctionType(Type *return_type, std::vector<Type *> arguments,
                           FuncFlags flags)
    : return_type(return_type), arguments(arguments), flags(flags) {}
LLVMTypeRef FunctionType::llvm_type() {
  LLVMTypeRef *llvm_args = new LLVMTypeRef[arguments.size()];
  for (size_t i = 0; i < arguments.size(); i++)
    llvm_args[i] = arguments[i]->llvm_type();
  return LLVMFunctionType(return_type->llvm_type(), llvm_args, arguments.size(),
                          flags.is_vararg);
}
TypeType FunctionType::type_type() { return TypeType::Function; }
bool FunctionType::eq(Type *other) {
  if (PointerType *ptr = dynamic_cast<PointerType *>(other))
    return this->eq(ptr->get_points_to());
  FunctionType *other_f = dynamic_cast<FunctionType *>(other);
  if (!other_f || other_f->arguments.size() != arguments.size() ||
      other_f->flags.neq(flags) || other_f->return_type->neq(return_type))
    return false;
  for (size_t i = 0; i < arguments.size(); i++)
    if (other_f->arguments[i]->neq(arguments[i]))
      return false;
  return true;
};
bool FunctionType::castable_to(Type *other) { return eq(other); };
std::string FunctionType::stringify() {
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
size_t FunctionType::_hash() {
  return hash(arguments) ^ hash(arguments.size()) ^ hash(TypeType::Function);
}