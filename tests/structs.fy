include "c/stdlib"
include "c/stdio"
struct Num { x: int }
struct Selfref { parent: *Selfref, data: int }

fun print(n: int)
	printf("%d"c, n)

fun heap()
	new Num { x = 1 }
fun stack()
	create Num { x = 2 }

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
	let top = create Selfref { data = 5 }
	let middle = create Selfref { parent = &top }
	let bottom = create Selfref { parent = &middle }
	let curr = &bottom
	while(curr.parent != null as *Selfref)
		curr = curr.parent
	print(curr.data)
	0
}
