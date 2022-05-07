// does the same thing as 03-for-loop, but using a while loop instead of a for loop.
include "std/io"
include "std/utils"

fun main() {
	let str = "Hello World!"
	const length = len(str)
	let i = 0 as uint_ptrsize // similar to size_t
	while (i < length) {
		print(str[i])
		i += 1
	}
	else // else runs if the first check of i<length fails.
		print("`str` is empty!")
	0
}