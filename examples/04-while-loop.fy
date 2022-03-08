declare fun putchar(ch: char): int
declare const stderr: *void
declare fun fputs(str: *char, stream: *void): int
declare fun strlen(str: *char): int
fun main() {
  const str: *char = "Hello World!"
  const len = strlen(str)
  let i = 0i
  # Iterate over all the characters in `str`, printing each individually. 
  while (i < len) {
    putchar(*(str + i))
    i = i + 1i
  } # else runs if the first check of i<len fails.
    # In this case that's when `str` has a length of 0, try setting it to ""!
    else fputs("`str` is empty!", stderr)
  # return value of main is inferred based off of 0i (int32)
  0i
}