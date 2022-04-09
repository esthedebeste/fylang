include "std/io"
// int32 is aliased to number
type number = int32

fun main(): number {
	let a: number = 2 * 4
	prints("2 * 4 = ")
	printn(a)
	0
}