include "c/stdio"
fun print(t: generic A)
	if(type A == *char)
		printf("%s", t)
	else if(type A == int)
		printf("%d", t)

fun main() {
	const a = "hello "
	const b = 1
	print(a)
	print(b)
	0
}