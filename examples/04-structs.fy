include "std/io"

struct String {
	pointer: *char,
	length: unsigned int
}
// Iterates over all characters in `str`, writing them all to stderr individually.
fun print_to_stdout(str: *String) {
	let i = 0i
	while(i < str.length) {
		// Access string indexes with ptr[offset]
		eputc(str.pointer[i])
		i = i + 1i
	}
}
fun main() {
	// Create a new instance of the String struct (this is a *String)
	const str = new String { pointer = "Hello from structs.fy!", length = 22i }
	print_to_stdout(str)
	0i
}