include "c/stdio"
include "types.fy"


fun(char) print() putchar(this)
fun(char[]) print() printf("%s", this)
fun(int32) print() printf("%d", this)
fun(int64) print() printf("%ld", this)
fun(int128) print() printf("%lld", this)
fun(uint32) print() printf("%u", this)
fun(uint64) print() printf("%lu", this)
fun(uint128) print() printf("%llu", this)
fun(float64) print() printf("%f", this)
fun(float32) print() (this as float64).print()

fun printc(ch: char) 	ch.print()
fun prints(str: char[])	str.print()
fun printd(x: double)	x.print()
fun printn(i: int)		i.print()

include "string.fy"
fun(*String) print_to(stream: *FILE)
	fwrite(this.chars, 1, this.length, stream)

fun(*String) print()
	this.print_to(stderr)