// does the same thing as 03-for-loop, but using a while loop instead of a for loop.
include "std/io"
include "std/string"

fun main() {
	const str = "Hello World!"
	let i = 0
	while (i < str.length) {
		print(str[i])
		i += 1
	}
	else // else runs if the first check of i<str.length fails.
		print("`str` is empty!")
	0
}