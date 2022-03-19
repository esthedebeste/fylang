include "c/ctype.fy"
include "types.fy"

fun(char) is_alnum(): bool
	isalnum(this)
fun(char) is_alpha(): bool
	isalpha(this)
fun(char) is_cntrl(): bool
	iscntrl(this)
fun(char) is_digit(): bool
	isdigit(this)
fun(char) is_graph(): bool
	isgraph(this)
fun(char) is_lower(): bool
	islower(this)
fun(char) is_print(): bool
	isprint(this)
fun(char) is_punct(): bool
	ispunct(this)
fun(char) is_space(): bool
	isspace(this)
fun(char) is_upper(): bool
	isupper(this)
fun(char) is_xdigit(): bool
	isxdigit(this)
fun(char) to_lower(): char
	tolower(this)
fun(char) to_upper(): char
	toupper(this)