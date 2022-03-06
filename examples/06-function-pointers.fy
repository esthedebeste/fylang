extern fun putchar(ch: char): int;
extern fun strlen(str: *char): int;

fun put_filtered(str: *char, predicate: *fun(char): bool): int {
  const len = strlen(str)
  let i = 0i
  while(i < len) { 
    # Access string indexes with *(ptr+offset)
    let char = *(str + i)
    if(predicate(char)) putchar(char) else 0i
    i = i + 1i
  } else 0i
  0i
}

fun is_alphanum(ch: char): bool
  (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')

fun main(): int {
  let str = "F@#(*i@#&l!#*(&t@$&*e!@)r@#()e{:><}d#!@*("
  # We pass the is_alphanum function to put_filtered as a predicate
  put_filtered(str, is_alphanum)
  0i
}