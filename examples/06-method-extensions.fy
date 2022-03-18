include "std/io"

// You can create method extensions on a type by specifying that type after `fun`,
// after that you can use these functions as methods on values of that type.
fun(int) double()
    this * 2

fun main() {
    eputs("4.double() => ")
    eputn((4).double())
    0
}