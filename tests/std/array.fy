include "std/io"
include "std/array"

fun uneven(x: int, i: uint_ptrsize): bool
	x % 2 == 1

fun double(x: int, i: uint_ptrsize): int
	x * 2

fun main() {
	let arr: Array<int>
	arr.init()
	arr.push(2)
	arr.push(3)
	arr.push(1)
	print("arr: ")
	print("\n - length: ") print(arr.length)
	print("\n - at(-1): ") print(arr.at(-1))
	print("\n - at( 0): ") print(arr.at(0))
	print("\n - at( 1): ") print(arr.at(1))
	print("\n - at( 5): ") print(arr.at(5))
	const filtered = arr.filter(uneven)
	print("\nfiltered: ")
	print("\n - length: ") print(filtered.length)
	print("\n -  at(0): ") print(filtered.at(0))
	print("\n -  at(1): ") print(filtered.at(1))
	const doubled = arr.map(double)
	print("\ndoubled: ")
	print("\n - length: ") print(doubled.length)
	print("\n - at(-1): ") print(doubled.at(-1))
	print("\n - at( 0): ") print(doubled.at(0))
	print("\n - at( 1): ") print(doubled.at(1))
	0
}