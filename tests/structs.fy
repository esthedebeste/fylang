include "std/io"
struct Num { x: int }

fun heap(): *Num
	new Num { x = 1 }
fun stack(): Num {
	let b: Num
	b.x = 2
	b
}

fun tuple() 
   *(3, )

fun tuple_ptr() 
   new (4, )

fun main() {
	let a = heap()
	eputn(a.x)
	let b = stack()
	eputn(b.x)
	let c = tuple()
	eputn(c.0)
	let d = tuple_ptr()
	eputn(d.0)
	0
}