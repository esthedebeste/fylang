include "std/std"

fun main() {
	let str = "hello world!"
	# ptr[offset] and ptr+offset can be used.
	str[0] = 'H'
	str+6  = 'W'
	eputs(str)
	0i
}