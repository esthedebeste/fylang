include "c/stdio"
fun print(t: generic A)
	if(type A == char[])
		printf("%s", t)
	else if(type A == int)
		printf("%d", t)
	else {
		DUMP A
		printf("Didn't know how to print t")
	}

fun(generic A) print()
	if(type A == char[])
		printf("%s", this)
	else
		printf("Expected (int) print to override.")

fun(int) print()
	printf("%d", this)

fun main() {
	const a: char[] = "hello "
	const b = 1
	print(a)
	print(b)
	a.print()
	b.print()
	0
}