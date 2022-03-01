extern eputs(str: *char): int
fun main() {
    let str = "hello world!"
    # Instead of ptr[offset], ptr+offset is used.
    # Note that offset is NOT a byte-shift and does take type-sizes into consideration.
    str+0 = 'H'
    str+6 = 'W'
    eputs(str)
    0i
}

