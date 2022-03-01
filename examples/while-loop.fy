extern putchar(ch: char): int
extern eputs(str: *char): int
extern strlen(str: *char): int
fun main() {
  # "str" is a *char ending with a NUL-byte
  let str = "Hello World!"
  let i: int = 0
  let len = strlen(str)
  # Iterate over all the characters in `str`, printing each individually. 
  while (i < len) {
    putchar(*(str + i))
    i = i + 1
  } # else runs if the first check of i<len fails.
    # In this case that's when `str` has a length of 0, try setting it to ""!
    else eputs("`str` is empty!")
  # return value of main is inferred based off of 0i (int32)
  0i
}