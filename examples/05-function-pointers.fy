include "std/io"
include "std/string"

// in arguments, functions can be annotated as
// `*fun(arg_type1, arg_type2, ...): return_type`
fun print_filtered(str: String, predicate: *fun(char): bool) {
	for(let i = 0; i < str.length; i += 1) {
		let char = str[i]
		if(predicate(char))
			print(char)
	}
}

fun is_alphanum(ch: char)
	(ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')

fun main() {
	const str = "F@#(*i@#&l!#*(&t@$&*e!@)r@#()e{:><}d#!@*("
	// We pass the is_alphanum function to put_filtered as a predicate
	print_filtered(str, is_alphanum) // "Filtered"
	0
}