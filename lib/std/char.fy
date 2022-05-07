include "types.fy"

// function(char)
fun is_lower(ch: char) ch >= 'a' && ch <= 'z'
fun is_upper(ch: char) ch >= 'A' && ch <= 'Z'
fun is_alpha(ch: char) ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z'
fun is_digit(ch: char) ch >= '0' && ch <= '9'
fun is_alphanumeric(ch: char) is_alpha(ch) || is_digit(ch)
fun is_control(ch: char) ch < ' ' || ch == 127b
fun is_graphic(ch: char) ch >= '!' && ch <= '~'
fun is_printable(ch: char) ch >= ' ' && ch <= '~'
fun is_punctuation(ch: char) (ch >= '!' && ch <= '/') || (ch >= ':' && ch <= '@') || (ch >= '[' && ch <= '`') || (ch >= '{' && ch <= '~')
fun is_space(ch: char) ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'
fun is_hexdigit(ch: char) is_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')

// char.method()
inline fun(char) is_lower() is_lower(this)
inline fun(char) is_upper() is_upper(this)
inline fun(char) is_alpha() is_alpha(this)
inline fun(char) is_digit() is_digit(this)
inline fun(char) is_alphanumeric() is_alphanumeric(this)
inline fun(char) is_control() is_control(this)
inline fun(char) is_graphic() is_graphic(this)
inline fun(char) is_printable() is_printable(this)
inline fun(char) is_punctuation() is_punctuation(this)
inline fun(char) is_space() is_space(this)
inline fun(char) is_hexdigit() is_hexdigit(this)

fun to_lower(ch: char) if(is_upper(ch)) { ch - 'A' + 'a' } else ch
fun to_upper(ch: char) if(is_lower(ch)) { ch - 'a' + 'A' } else ch

inline fun(char) to_lower() to_lower(this)
inline fun(char) to_upper() to_upper(this)