include "c/stdlib"
include "c/stdio"
struct Num { x: int }

fun print(n: int)
	printf("%d", n)

fun heap()
	new Num { x = 1 }
fun stack() {
	let b: Num
	b.x = 2
	b
}

fun tuple()
   (3, )

fun tuple_ptr()
   new (4, )

fun main() {
	let a = heap()
	print(a.x)
	ASSERT_TYPE typeof(a) == *Num
	let b = stack()
	print(b.x)
	ASSERT_TYPE typeof(b) == Num
	let c = tuple()
	print(c.0)
	ASSERT_TYPE typeof(c) == { int }
	let d = tuple_ptr()
	print(d.0)
	ASSERT_TYPE typeof(d) == *{ int }
	0
}