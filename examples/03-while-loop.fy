// does the same thing as 03-for-loop, but using a while loop instead of a for loop.
include "std/io"
include "std/string"

fun main() {
	const str = "Hello World!"
	const len = str.length()
	let i = 0
	// Iterate over all the characters in `str`, printing each individually. 
	while (i < len) {
		eputc(str[i])
		i += 1
	}
	else // else runs if the first check of i<len fails.
		eputs("`str` is empty!")
	0
}