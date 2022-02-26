extern puts(str: *char): int;
fun main() {
  # "str"c is a *char
  puts("Hello World!"c)
  # return value of main is inferred based off of 0i (int32)
  0i
}