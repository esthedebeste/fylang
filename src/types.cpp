#include "utils.cpp"
#pragma once
static LLVMTypeRef int_1_type = LLVMInt1Type(); // AKA bool
static LLVMTypeRef int_8_type = LLVMInt8Type(); // AKA byte, char
static LLVMTypeRef int_16_type = LLVMInt16Type();
static LLVMTypeRef int_32_type = LLVMInt32Type();    // AKA int
static LLVMTypeRef int_64_type = LLVMInt32Type();    // AKA long
static LLVMTypeRef int_128_type = LLVMInt128Type();  // AKA long long
static LLVMTypeRef float_16_type = LLVMHalfType();   // AKA half
static LLVMTypeRef float_32_type = LLVMFloatType();  // AKA float
static LLVMTypeRef float_64_type = LLVMDoubleType(); // AKA double
static LLVMTypeRef float_128_type = LLVMFP128Type();

/// Base type class.
class Type
{
public:
    virtual ~Type() {}
    virtual unsigned int get_byte_size() = 0;
    virtual unsigned int get_bit_size() = 0;
    virtual LLVMTypeRef llvm_type() = 0;
    virtual bool eq(Type *other) = 0;
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
        this->byte_size = bits * 8;
    }
    NumType(char *bits_str, unsigned int bits_str_len, bool is_floating, bool is_signed) : is_floating(is_floating), is_signed(is_signed)
    {
        bits = parse_pos_int(bits_str, bits_str_len, 10);
        byte_size = bits * 8;
    }
    unsigned int get_byte_size()
    {
        return byte_size;
    }
    unsigned int get_bit_size()
    {
        return bits;
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
        default:
            fprintf(stderr, "TypeError: Unknown numerical type (%d bits, %d floating, %d signed)", bits, is_floating, is_signed);
            exit(1);
            break;
        }
    }
    bool eq(Type *other)
    {
        if (NumType *other_n = dynamic_cast<NumType *>(other))
            return other_n->bits == bits && other_n->is_floating == is_floating && other_n->is_signed == is_signed;
        return false;
    }
};
class PointerType
{
public:
    Type *points_to;
    PointerType(Type *points_to) : points_to(points_to) {}
    Type *get_points_to()
    {
        return points_to;
    }
    bool eq(Type *other)
    {
        if (PointerType *other_n = dynamic_cast<PointerType *>(other))
            return other_n->points_to->eq(this->points_to);
        return false;
    }
};
class FunctionType : public Type
{
public:
    Type *return_type;
    Type **arguments;
    unsigned int arguments_len;

    FunctionType(Type *return_type, Type **arguments, unsigned int arguments_len) : return_type(return_type), arguments(arguments), arguments_len(arguments_len) {}
    unsigned int get_byte_size()
    {
        error("functions don't have a byte size");
        return 0;
    }
    unsigned int get_bit_size()
    {
        error("functions don't have a bit size");
        return 0;
    }
    LLVMTypeRef llvm_type()
    {
        error("functions don't have LLVM types");
        return nullptr;
    }
    virtual bool eq(Type *other)
    {
        // functions are unique
        return other == this;
    };
};
