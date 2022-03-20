include "std/io"
include "std/string"

fun main() {
	const str = "Hello World!"
	const len = str.length()
	// Iterate over all the characters in `str`, printing each individually. 
	for (let i = 0; i < len; i += 1) 
		// Access string indexes with ptr[offset]
		eputc(str[i])
	  // else runs if the first check of i<len fails.
	  // In this case that's when `str` has a length of 0, try setting it to ""!
	else 
		eputs("`str` is empty!")
	0
}

