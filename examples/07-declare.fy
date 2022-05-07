include "std/types"
// C functions and globals can be used from fy using 'declare'
declare const stdout: *unknown
declare fun fputs(str: *char, file: *unknown): int
fun main() {
	fputs("Hello World!"c, stdout)
	0
}

// most C functions are declared in c/[c-header-name].
// for example, 'include "c/string"' will include declarations of C string functions, such as strlen, strcpy, and strcat.
// "c/stdio" includes functions like puts, fread, and printf, but also contains constants such as stdin, stderr, and stdout.
// it is of course recommended to use std instead of declaring C functions and globals.

// this example doesn't work on windows because windows registers stdout differently.