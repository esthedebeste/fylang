include "std/io"

fun p0(str: generic S) {
	print(str)
	0
}
fun main() {
	if("hey" == "hey")
		p0("Array comparison works\n")
	else return 1
	if([1, 2, 3] + [4, 5, 6] == [5, 7, 9])
		p0("Array adding works\n")
	else return 1
	if([1, 2, 3] + 1 == [2, 3, 4])
		p0("Array-item adding works\n")
	else return 1
	0
}
