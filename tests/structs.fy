include "std/io"
struct Num { x: int }

fun heap()
	new Num { x = 1 }
fun stack(): Num {
	let b: Num
	b.x = 2
	b
}
fun tuple() 
   *(3, )

fun main() {
	let a: *Num = heap()
	eputn(a.x)
	let b: Num = stack()
	eputn(b.x)
	let c = tuple()
	eputn(c.0)
}