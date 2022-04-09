include "std/io"
include "std/string"

fun main() {
	const str = "Hello World!"
	const len = str.length()
	for (let i = 0; i < len; i += 1)
		printc(str[i])
	else // else runs if the first check of i<len fails.
		prints("`str` is empty!")
	0
}