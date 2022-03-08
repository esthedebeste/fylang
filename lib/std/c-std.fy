# declares C globals
struct unknown {}
declare const stderr: *unknown
declare fun fputc(ch: char, stream: *unknown): int
declare fun fputs(str: *char, stream: *unknown): int
declare fun puts(str: *char): int
declare fun putchar(ch: char): int
declare fun strlen(str: *char): int
