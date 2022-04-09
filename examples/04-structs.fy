include "std/io"

struct String {
	pointer: *char,
	length: unsigned int
}
fun print_to_stdout(str: *String)
	for(let i = 0; i < str.length; i += 1)
		printc(str.pointer[i])

fun main() {
	const str: *String = new String { pointer = "Hello from structs.fy!", length = 22 }
	print_to_stdout(str)
	0
}