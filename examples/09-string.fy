include "std/string"
include "std/io"

fun main() {
	let a = create_string("Hello ")
	let b = create_string("World!")
	let hw = a.concat(b).lowercase()
	hw.print()
	0
}