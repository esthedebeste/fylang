include "std/io"
include "std/utils"

fun main() {
	let str = "Hello World!"
	const length = len(str)
	for (let i = 0; i < length; i += 1)
		print(str[i])
	else // else runs if the first check of i<length fails.
		print("`str` is empty!")
	0
}