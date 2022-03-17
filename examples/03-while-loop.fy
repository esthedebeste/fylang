include "std/io"
include "c/string"

fun main() {
	const str: *char = "Hello World!"
	const len = strlen(str)
	let i = 0i
	// Iterate over all the characters in `str`, printing each individually. 
	while (i < len) {
		// Access string indexes with ptr[offset]
		eputc(str[i])
		i = i + 1i
	} // else runs if the first check of i<len fails.
	  // In this case that's when `str` has a length of 0, try setting it to ""!
	  else eputs("`str` is empty!")
	// return value of main is inferred based off of 0i (int32)
	0i
}