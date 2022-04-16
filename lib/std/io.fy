include "c/stdio"
include "os/{os}/io"
include "types.fy"

fun(char) print_to(s: *FILE) putc(this, s)
fun(char[]) print_to(s: *FILE) fputs(this, s)
fun(int32) print_to(s: *FILE) fprintf(s, "%d", this)
fun(int64) print_to(s: *FILE) fprintf(s, "%ld", this)
fun(int128) print_to(s: *FILE) fprintf(s, "%lld", this)
fun(uint32) print_to(s: *FILE) fprintf(s, "%u", this)
fun(uint64) print_to(s: *FILE) fprintf(s, "%lu", this)
fun(uint128) print_to(s: *FILE) fprintf(s, "%llu", this)
fun(float64) print_to(s: *FILE) fprintf(s, "%f", this)
fun(float32) print_to(s: *FILE) (this as float64).print()
fun(typeof "") print_to(s: *FILE) fwrite(this.chars, 1, this.length, s)

fun(generic X) print(): uint_ptrsize this.print_to(stdout())
fun print(x: generic X): uint_ptrsize x.print()

fun printc(ch: char)	ch.print()
fun prints(str: char[])	str.print()
fun printd(x: double)	x.print()
fun printn(i: int)		i.print()