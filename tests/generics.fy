include "c/stdio"
include "os/{os}/io"

fun print(t: generic A)
	if(type A == *char[generic Len])
		printf("%s"c, t)
	else if(type A == int)
		printf("%d"c, t)

fun(generic A) print()
	if(typeof(this) == *char[generic Len])
		printf("%s"c, this)
	else
		puts("Expected (int) print to override."c)

fun(int) print()
	printf("%d"c, this)

fun main() {
	const a = "hello "c
	const b = 1
	print(a)
	print(b)
	a.print()
	b.print()
	0
}
