# declares C globals

declare const stderr: *void
declare fun fputc(ch: char, stream: *void): int
declare fun fputs(str: *char, stream: *void): int
