include "c/stdio.fy"
include "types.fy"

// eputc - takes a char and prints it to stderr
fun eputc(ch: char): int
    fputc(ch as int32, stderr)

// eputs - takes a string and prints it to stderr (without newline)
fun eputs(str: *char): int
    fputs(str, stderr)

/// eputd - takes a double and prints it to stderr
fun eputd(x: double): int 
    fprintf(stderr, "%f", x)

/// eputn - takes a i32 and prints it to stderr
fun eputn(i: int): int
    fprintf(stderr, "%d", i)