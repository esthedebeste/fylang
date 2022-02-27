extern putchar(ch: char): int
extern eputs(str: *char): int
fun get(ptr: *char, index: int)
    # offset ptr by index, and then dereference
    *(ptr + index)
fun putnth(ptr: *byte, index: int)
    putchar(get(ptr, index))

fun main() {
    let str = "hello world! 🖤"c
    eputs(str + 13)
    putnth(str, 11)
    0i
}

