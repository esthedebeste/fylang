# C functions/libraries can be used from fy
extern fun puts(str: *char): int;
fun main() {
  # "str" is a *char ending with a NUL-byte
  puts("Hello World!")
  # return value of main is inferred based off of 0i (int32)
  0i
}