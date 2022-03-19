include "c/stdio"
include "types.fy"
include "std/string.fy"

// eputc - takes a char and prints it to stderr
fun eputc(ch: char)
    fputc(ch as int32, stderr)

// eputs - takes a string and prints it to stderr (without newline)
fun eputs(str: *char)
    fputs(str, stderr)

// eputd - takes a double and prints it to stderr
fun eputd(x: double)
    fprintf(stderr, "%f", x)

// eputn - takes a i32 and prints it to stderr
fun eputn(i: int)
    fprintf(stderr, "%d", i)

fun(*String) print_to(stream: *FILE)
   fwrite(this.chars, CHAR_SIZE, this.length, stream)

fun(*String) print()
    this.print_to(stderr)