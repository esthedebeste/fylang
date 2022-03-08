declare fun fputs(str: *char, out: *void): void
# *void is useful for cases where you don't feel like typing what a pointer refers to (in this case, the C file struct) 
declare const stderr: *void
fun main() {
    fputs("Hi from void-pointer.fy!", stderr)
    0i
}