include "c/stdio"

const shadow: int = 7
space SpaceTest {
	const shadow: int = 5
	fun print_bang() putchar('!')
	space B {
		fun print_ok() {
			puts("OK"c)
			printf("%d"c, shadow)
			print_bang()
		}
	}
}

fun main() {
	SpaceTest::B::print_ok()
	printf("%d\n"c, shadow)
	0
}
