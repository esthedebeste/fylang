include "c/stdio.fy"
include "cpp-implemented-std.fy"

// eputc - takes a char and prints it to stderr
fun eputc(ch: char): int
    fputc(ch, stderr)

// eputs - takes a string and prints it to stderr (without newline)
fun eputs(str: *char): int
    fputs(str, stderr)