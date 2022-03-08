type char = int8
// C functions can be used from fy using 'declare'
declare fun puts(str: *char): int
fun main(): int {
	// "str" is a *char ending with a NUL-byte
	puts("Hello World!")
	0i
}