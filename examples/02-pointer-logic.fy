extern fun puts(str: *char): void
fun main() {
    let str = "hello world!"
    # Instead of ptr[offset], ptr+offset is used.
    # Note that offset is NOT a byte-shift and does take type-sizes into consideration.
    str+0 = 'H'
    str+6 = 'W'
    puts(str)
    0i
}

