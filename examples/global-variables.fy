extern fun eputs(str: *char): void
extern fun eputn(num: int): void
const str = "Hello World!\n"
const num = 4i
fun main() {
    eputs(str)
    eputn(num)
    0i
}