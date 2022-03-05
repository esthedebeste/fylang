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
  ((ch > 'a'-1) & (ch < 'z'+1)) | ((ch > 'A'-1) & (ch < 'Z'+1)) | ((ch > '0'-1) & (ch < '9'+1))

fun main(): int {
  let str = "F@#(*i@#&l!#*(&t@$&*e!@)r@#()e{:><}d#!@*("
  # We pass the is_alphanum function to put_filtered as a predicate
  put_filtered(str, &is_alphanum)
  0i
}

