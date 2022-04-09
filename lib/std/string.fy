include "c/string"
include "c/stdlib"
include "types.fy"
include "char.fy"

fun(char[]) length()
	strlen(this)
fun(char[]) clone()
	strdup(this)

struct String {
	chars: char[],
	length: uint_ptrsize
}

fun alloc_chars(amount: uint_ptrsize): char[]
	calloc(amount, 1) // null-initialized

fun realloc_chars(ptr: char[], new_size: uint_ptrsize): char[]
	realloc(ptr, new_size)

fun create_string(chars: char[])
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
	for (let i: uint_ptrsize = 0; i < this.length; i += 1)
		uppered[i] = transformer(this.chars[i])
	new String { chars = uppered, length = this.length }
}

fun(*String) filter(predicate: *fun(char): bool): *String {
	const result = alloc_chars(this.length)
	let len: uint_ptrsize = 0
	for (let i: uint_ptrsize = 0; i < this.length; i += 1) {
		const char = this.chars[i]
		if (predicate(char)) {
			result[len] = char
			len += 1
		}
	}
	new String { chars = realloc_chars(result, len), length = len }
}

fun(*String) uppercase(): *String
	this.transform(&toupper)

fun(*String) lowercase(): *String
	this.transform(&tolower)

fun streql(a: char[], b: char[], len: uint_ptrsize): bool {
	for (let i = 0 as uint_ptrsize; i < len; i += 1)
		if (a[i] != b[i])
			return false
	true
}

fun(*String) starts_with(prefix: *String): bool
	if(this.length < prefix.length) false
	else streql(this.chars, prefix.chars, prefix.length)

fun(*String) ends_with(postfix: *String): bool
	if(this.length < postfix.length)
		false
	else {
		const offset = this.length - postfix.length
		for (let i = postfix.length; i > 0; i -= 1)
			if (this.chars[offset + i - 1] != postfix.chars[i - 1])
				return false
		true
	}