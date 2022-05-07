include "std/io"
include "std/string"

fun main() {
	const a = str("Hello ")
	const b = str("World!")
	const hw = a.concat(b).lowercase()
	print(hw)
	0
}