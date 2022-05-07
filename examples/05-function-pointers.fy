include "std/io"
include "std/utils"

// in arguments, functions can be annotated as
// `*fun(arg_type1, arg_type2, ...): return_type`
fun print_filtered(str: *char, length: uint_ptrsize, predicate: *fun(char): bool) {
	for(let i = 0; i < length; i += 1) {
		const char = str[i]
		if(predicate(char))
			print(char)
	}
}

fun is_alphanum(ch: char)
	(ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')

fun main() {
	let string = "F@#(*i@#&l!#*(&t@$&*e!@)r@#()e{:><}d#!@*("
	let length = len(string)
	// We pass the is_alphanum function to put_filtered as a predicate
	print_filtered(string, length, is_alphanum) // "Filtered"
	0
}