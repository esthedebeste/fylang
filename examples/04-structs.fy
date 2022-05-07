include "std/io"
include "c/stdlib"

struct String {
	chars: *char,
	length: unsigned int
}

fun print_to_stdout(str: *String)
	for(let i = 0; i < str.length; i += 1)
		print(str.chars[i])

fun main() {
	const str1: *String = new String { chars = "Hello from "c, length = 11 }
	print_to_stdout(str1)
	let str2: String = create String { chars = "structs.fy!"c, length = 11 }
	print_to_stdout(&str2)
	0
}