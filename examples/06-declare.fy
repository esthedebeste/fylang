include "std/types"
// C functions and globals can be used from fy using 'declare'
declare const stdout: *unknown
declare fun fputs(str: *char, file: *unknown): int
fun main(): int {
	fputs("Hello World!", stdout)
	0i
}

// most C functions are declared in c/[c-header-name].
// for example, 'include "c/string"' will include declarations of C string functions, such as strlen, strcpy, and strcat.
// "c/stdio" includes functions like puts, fread, and printf, but also contains constants such as stdin, stderr, and stdout.