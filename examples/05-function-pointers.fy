include "std/std"
include "c/string"

// in arguments, functions can be annotated as
// `*fun(arg_type1, arg_type2, ...): return_type`
fun put_filtered(str: *char, predicate: *fun(char): bool) {
	const len = strlen(str) as int32
	let i = 0i
	while(i < len) {
		// Access string indexes with *(ptr+offset)
		let char = str[i]
		if(predicate(char)) putchar(char) else 0i
		i = i + 1i
	} else 0i
	0i
}

fun is_alphanum(ch: char)
	(ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')

fun main() {
	let str = "F@#(*i@#&l!#*(&t@$&*e!@)r@#()e{:><}d#!@*("
	// We pass the is_alphanum function to put_filtered as a predicate
	put_filtered(str, is_alphanum) // "Filtered"
	0i
}