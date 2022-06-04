include "std/io"

// todo: add a better way to initialize arrays
fun arr(a: generic E, b: E, c: E): E[3]
	(a, b, c)

fun p0(str: generic S) {
	print(str)
	0
}
fun main() {
	if("hey" == "hey")
		p0("Array comparison works\n")
	else return 1
	const res = arr(1, 2, 3) + arr(4, 5, 6)
	if(res == arr(5, 7, 9))
		p0("Array adding works\n")
	else return 1
	0
}
