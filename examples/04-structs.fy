include "std/io"

struct String {
	pointer: *char,
	length: unsigned int
}
// Iterates over all characters in `str`, writing them all to stderr individually.
fun print_to_stdout(str: *String) 
	for(let i = 0; i < str.length; i += 1)
		eputc(str.pointer[i])

fun main() {
	// Create a new instance of the String struct (this is a *String)
	const str = new String { pointer = "Hello from structs.fy!", length = 22 }
	print_to_stdout(str)
	0
}