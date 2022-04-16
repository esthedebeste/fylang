include "std/io"
include "std/string"

fun main() {
	const str = "Hello World!"
	for (let i = 0; i < str.length; i += 1)
		print(str[i])
	else // else runs if the first check of i<str.length fails.
		print("`str` is empty!")
	0
}