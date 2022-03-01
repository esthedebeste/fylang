extern fun putchar(ch: char): int
extern fun eputs(str: *char): int
extern fun strlen(str: *char): int
fun main() {
  # "str" is a *char ending with a NUL-byte
  const str: *char = "Hello World!"
  const len = strlen(str)
  let i: int = 0
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