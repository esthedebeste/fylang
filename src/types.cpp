#include "utils.cpp"
#pragma once

static LLVMTypeRef void_type = LLVMStructCreateNamed(LLVMGetGlobalContext(), "void");
static LLVMTypeRef int_1_type = LLVMInt1Type(); // AKA bool
static LLVMTypeRef int_8_type = LLVMInt8Type(); // AKA byte, char
static LLVMTypeRef int_16_type = LLVMInt16Type();
static LLVMTypeRef int_32_type = LLVMInt32Type();    // AKA int
static LLVMTypeRef int_64_type = LLVMInt64Type();    // AKA long
static LLVMTypeRef int_128_type = LLVMInt128Type();  // AKA long long
static LLVMTypeRef float_16_type = LLVMHalfType();   // AKA half
static LLVMTypeRef float_32_type = LLVMFloatType();  // AKA float
static LLVMTypeRef float_64_type = LLVMDoubleType(); // AKA double
static LLVMTypeRef float_128_type = LLVMFP128Type();

enum TypeType
{
    Void = 0,
    Number = 1,
    Pointer = 2,
    Function = 3,
    Array = 4,
    Struct = 5
};
static const char *tt_to_str(TypeType tt)
{
    switch (tt)
    {
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
    }
};
/// Base type class.
class Type
{
public:
    virtual ~Type() {}
    virtual LLVMTypeRef llvm_type() = 0;
    virtual TypeType type_type() = 0;
    virtual bool eq(Type *other)
    {
        return this->type_type() == other->type_type();
    };
    virtual bool neq(Type *other)
    {
        return !eq(other);
    }
    virtual void log_diff(Type *other)
    {
        log_type_diff(other);
    }
    bool log_type_diff(Type *other)
    {
        if (this->type_type() == other->type_type())
            return false;
        fprintf(stderr, "\n\t- type: A=%s, B=%s", tt_to_str(this->type_type()), tt_to_str(other->type_type()));
        return true;
    };
};

class VoidType : public Type
{
public:
    VoidType() {}
    LLVMTypeRef llvm_type() { return void_type; }
    TypeType type_type() { return TypeType::Void; }
};

class NumType : public Type
{
public:
    unsigned int byte_size;
    unsigned int bits;
    bool is_floating;
    bool is_signed;
    NumType(unsigned int bits, bool is_floating, bool is_signed) : bits(bits), is_floating(is_floating), is_signed(is_signed)
    {
        byte_size = bits * 8;
    }
    NumType(char *bits_str, unsigned int bits_str_len, bool is_floating, bool is_signed) : is_floating(is_floating), is_signed(is_signed)
    {
        bits = parse_pos_int(bits_str, bits_str_len, 10);
        byte_size = bits * 8;
    }
    LLVMTypeRef llvm_type()
    {
        switch (bits)
        {
        case 1:
            if (is_floating)
                error("floating 1-bit numbers don't exist");
            return int_1_type;
        case 8:
            if (is_floating)
                error("floating 8-bit numbers don't exist");
            return int_8_type;
        case 16:
            if (is_floating)
                return float_16_type;
            return int_16_type;
        case 32:
            if (is_floating)
                return float_32_type;
            return int_32_type;
        case 64:
            if (is_floating)
                return float_64_type;
            return int_64_type;
        case 128:
            if (is_floating)
                return float_128_type;
            return int_128_type;
        }
        fprintf(stderr, "TypeError: Unknown numerical type (%d bits, %d floating, %d signed)", bits, is_floating, is_signed);
        exit(1);
    }
    TypeType type_type()
    {
        return TypeType::Number;
    }
    bool eq(Type *other)
    {
        if (NumType *other_n = dynamic_cast<NumType *>(other))
            return other_n->bits == bits && other_n->is_floating == is_floating && other_n->is_signed == is_signed;
        return false;
    }
    void log_diff(Type *other)
    {
        if (log_type_diff(other))
            return;
        NumType *b = dynamic_cast<NumType *>(other);
        unsigned int a_bits = this->bits;
        unsigned int b_bits = b->bits;
        if (a_bits != b_bits)
            fprintf(stderr, "\n\t- bits: A=%d, B=%d", a_bits, b_bits);
        bool a_flt = this->is_floating;
        bool b_flt = b->is_floating;
        if (a_flt != b_flt)
            fprintf(stderr, "\n\t- floating: A=%d, B=%d", a_flt, b_flt);
        bool a_sgn = this->is_signed;
        bool b_sgn = b->is_signed;
        if (a_sgn != b_sgn)
            fprintf(stderr, "\n\t- signed: A=%d, B=%d", a_sgn, b_sgn);
    }
};
class PointerType : public Type
{
public:
    Type *points_to;
    PointerType(Type *points_to) : points_to(points_to)
    {
    }
    Type *get_points_to()
    {
        return points_to;
    }
    LLVMTypeRef llvm_type()
    {
        return LLVMPointerType(this->points_to->llvm_type(), 0);
    }
    TypeType type_type()
    {
        return TypeType::Pointer;
    }
    bool eq(Type *other)
    {
        if (PointerType *other_n = dynamic_cast<PointerType *>(other))
            return other_n->points_to->eq(this->points_to);
        return false;
    }
    void log_diff(Type *other)
    {
        if (log_type_diff(other))
            return;
        PointerType *b = dynamic_cast<PointerType *>(other);
        return this->get_points_to()->log_diff(b->get_points_to());
    }
};
class ArrayType : public Type
{
public:
    Type *elem;
    unsigned int count;
    ArrayType(Type *elem, unsigned int count) : elem(elem), count(count)
    {
    }
    Type *get_elem_type()
    {
        return elem;
    }
    unsigned int get_elem_count()
    {
        return count;
    }
    LLVMTypeRef llvm_type()
    {
        return LLVMArrayType(elem->llvm_type(), count);
    }
    TypeType type_type()
    {
        return TypeType::Array;
    }
    bool eq(Type *other)
    {
        if (ArrayType *other_arr = dynamic_cast<ArrayType *>(other))
            return other_arr->elem->eq(this->elem) && other_arr->count == this->count;
        return false;
    }
    void log_diff(Type *other)
    {
        if (log_type_diff(other))
            return;
        ArrayType *b = dynamic_cast<ArrayType *>(other);
        unsigned int a_count = this->count;
        unsigned int b_count = b->count;
        if (a_count != b_count)
            fprintf(stderr, "\n\t- amount: A=%d, B=%d", a_count, b_count);
        b->elem->log_diff(this->elem);
    }
};
class StructType : public Type
{
    LLVMTypeRef llvm_struct_type;

public:
    char *name;
    unsigned int name_len;
    Type **types;
    char **keys;
    unsigned int *key_lengths;
    unsigned int count;
    StructType(char *name, unsigned int name_len, char **keys, unsigned int *key_lengths, Type **types, unsigned int count) : name(name), name_len(name_len), keys(keys), key_lengths(key_lengths), types(types), count(count)
    {
        LLVMTypeRef *llvm_types = alloc_arr<LLVMTypeRef>(count);
        for (unsigned int i = 0; i < count; i++)
            llvm_types[i] = types[i]->llvm_type();
        llvm_struct_type = LLVMStructCreateNamed(curr_ctx, name);
        LLVMStructSetBody(llvm_struct_type, llvm_types, count, true);
    }
    Type *get_elem_type(unsigned int index)
    {
        if (index >= count)
            error("Struct get_elem_type index out of bounds");
        return types[index];
    }
    unsigned int get_index(char *name, unsigned int name_len)
    {
        for (unsigned int i = 0; i < count; i++)
        {
            if (key_lengths[i] == name_len)
            {
                for (unsigned int j = 0; j < name_len; j++)
                    if (keys[i][j] != name[j])
                        continue;
                return i;
            }
        }
        error("Struct does not have key");
        return -1;
    }
    LLVMTypeRef llvm_type()
    {
        return llvm_struct_type;
    }
    TypeType type_type()
    {
        return TypeType::Struct;
    }
    bool eq(Type *other)
    {
        // structs are unique
        return this == other;
    }
    void log_diff(Type *other)
    {
        if (this != other)
            fprintf(stderr, "Structs are unique.");
    }
};
class FunctionType : public Type
{
public:
    Type *return_type;
    Type **arguments;
    unsigned int arguments_len;

    FunctionType(Type *return_type, Type **arguments, unsigned int arguments_len) : return_type(return_type), arguments(arguments), arguments_len(arguments_len)
    {
    }
    LLVMTypeRef llvm_type()
    {
        LLVMTypeRef *llvm_args = alloc_arr<LLVMTypeRef>(arguments_len);
        for (unsigned int i = 0; i < arguments_len; i++)
            llvm_args[i] = arguments[i]->llvm_type();
        // todo: vararg functions?
        return LLVMFunctionType(return_type->llvm_type(), llvm_args, arguments_len, false);
    }
    TypeType type_type()
    {
        return TypeType::Function;
    }
    bool eq(Type *other)
    {
        FunctionType *other_f = dynamic_cast<FunctionType *>(other);
        if (!other_f)
            return false;
        if (other_f->arguments_len != arguments_len)
            return false;
        if (other_f->return_type->neq(return_type))
            return false;
        for (unsigned int i = 0; i < arguments_len; i++)
            if (other_f->arguments[i]->neq(arguments[i]))
                return false;
        return true;
    };
};