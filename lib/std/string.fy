include "c/string"
include "c/stdlib"
include "types.fy"
include "char.fy"

const CHAR_SIZE = 1
fun(*char) length()
    strlen(this)
fun(*char) clone()
    strdup(this)

struct String {
    chars: *char,
    length: int_ptrsize
}

fun alloc_chars(amount: int_ptrsize): *char
    calloc(amount, CHAR_SIZE)

fun create_string(chars: *char)
    new String { chars = chars.clone(), length = chars.length() }

fun(*String) concat(other: *String): *String {
    const length = this.length + other.length
    const chars = alloc_chars(length + 1)
    memcpy(chars, this.chars, this.length)
    memcpy(chars + this.length, other.chars, other.length)
    new String { chars = chars, length = length }
}

fun(*String) transform(transformer: *fun(char): char): *String {
    let uppered = alloc_chars(this.length)
    let i = 0
    while (i < this.length) {
        uppered[i] = transformer(this.chars[i])
        i = i + 1
    }
    new String { chars = uppered, length = this.length }
}

fun(*String) uppercase(): *String 
    this.transform(&toupper)

fun(*String) lowercase(): *String
    this.transform(&tolower)


fun streql(a: *char, b: *char, len: int_ptrsize): bool {
    let i = 0 as int_ptrsize
    while (i < len) {
        if (a[i] != b[i])
            false
        i = i + 1
    }
    true
}

include "std/io"
fun(*String) starts_with(prefix: *String): bool
    if(this.length < prefix.length) false
    else streql(this.chars, prefix.chars, prefix.length)

fun(*String) ends_with(postfix: *String): bool
    if(this.length < postfix.length) false
    else {
        const offset = this.length - postfix.length
        let i = postfix.length
        while (i > 0) {
            if (this.chars[offset+i - 1] != postfix.chars[i - 1])
                return false
            i = i - 1
        }
        true
    }