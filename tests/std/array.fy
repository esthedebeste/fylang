include "std/io"
include "std/array"

type i = *{ int }
fun uneven(x: i, i: uint_ptrsize): bool
	x.0 % 2 == 1

fun double(x: i, i: uint_ptrsize): i
	new (x.0 * 2,)

fun(*Array) ati(i: int_ptrsize): int
	(this.at(i) as i).0

fun main() {
	const arr = create_array()
	arr.push(new (2, ))
	arr.push(new (3, ))
	arr.push(new (1, ))
	prints("arr: ")
	prints("\n - length: "); printn(arr.length);
	prints("\n - at(-1): "); printn(arr.ati(-1))
	prints("\n - at( 0): "); printn(arr.ati(0))
	prints("\n - at( 1): "); printn(arr.ati(1))
	const filtered = arr.filter(uneven)
	prints("\nfiltered: ")
	prints("\n - length: "); printn(filtered.length)
	prints("\n -  at(0): "); printn(filtered.ati(0))
	prints("\n -  at(1): "); printn(filtered.ati(1))
	const doubled = arr.map(double)
	prints("\ndoubled: ")
	prints("\n - length: "); printn(doubled.length)
	prints("\n - at(-1): "); printn(doubled.ati(-1))
	prints("\n - at( 0): "); printn(doubled.ati(0))
	prints("\n - at( 1): "); printn(doubled.ati(1))
	0
}