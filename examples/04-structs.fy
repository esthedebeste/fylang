include "std/io"
include "c/stdlib"

struct String {
	chars: char[],
	length: unsigned int
}
fun print_to_stdout(str: *String)
	for(let i = 0; i < str.length; i += 1)
		print(str.chars[i])

fun main() {
	const str: *String = new String { chars = "Hello from structs.fy!", length = 22 }
	print_to_stdout(str)
	0
}