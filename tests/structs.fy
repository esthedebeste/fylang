include "std/io"
struct Num { x: int }

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
	printn(a.x)
	ASSERT_TYPE typeof(a) == *Num
	let b = stack()
	printn(b.x)
	ASSERT_TYPE typeof(b) == Num
	let c = tuple()
	printn(c.0)
	ASSERT_TYPE typeof(c) == { int }
	let d = tuple_ptr()
	printn(d.0)
	ASSERT_TYPE typeof(d) == *{ int }
	0
}