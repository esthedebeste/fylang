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
    eputs("arr: ")
    eputs("\n - length: "); eputn(arr.length);
    eputs("\n - at(-1): "); eputn(arr.ati(-1))
    eputs("\n - at( 0): "); eputn(arr.ati(0))
    eputs("\n - at( 1): "); eputn(arr.ati(1))
    const filtered = arr.filter(uneven)
    eputs("\nfiltered: ")
    eputs("\n - length: "); eputn(filtered.length)
    eputs("\n -  at(0): "); eputn(filtered.ati(0))
    eputs("\n -  at(1): "); eputn(filtered.ati(1))
    const doubled = arr.map(double)
    eputs("\ndoubled: ")
    eputs("\n - length: "); eputn(doubled.length)
    eputs("\n - at(-1): "); eputn(doubled.ati(-1))
    eputs("\n - at( 0): "); eputn(doubled.ati(0))
    eputs("\n - at( 1): "); eputn(doubled.ati(1))
    0
}