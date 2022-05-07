include "c/stdio"
include "os/{os}/io"
include "types.fy"

inline fun(char) print_to(s: *FILE) putc(this, s)
inline fun(*char) print_to(s: *FILE) fputs(this, s)
inline fun(int32) print_to(s: *FILE) fprintf(s, "%d"c, this)
inline fun(int64) print_to(s: *FILE) fprintf(s, "%ld"c, this)
inline fun(int128) print_to(s: *FILE) fprintf(s, "%lld"c, this)
inline fun(uint32) print_to(s: *FILE) fprintf(s, "%u"c, this)
inline fun(uint64) print_to(s: *FILE) fprintf(s, "%lu"c, this)
inline fun(uint128) print_to(s: *FILE) fprintf(s, "%llu"c, this)
inline fun(float64) print_to(s: *FILE) fprintf(s, "%f"c, this)
inline fun(float32) print_to(s: *FILE) (this as float64).print()
inline fun(*generic Elem[generic Length]) print_to(s: *FILE)
	fwrite(this, sizeof Elem, Length, s)
inline fun(generic Elem[generic Length]) print_to(s: *FILE)
	(&let ptr = this).print_to(s)

inline fun(generic X) print():  uint_ptrsize this.print_to(stdout)
inline fun print(x: generic X): uint_ptrsize x.print()


inline fun printc(ch: char)	ch.print()
inline fun prints(str: *char)	str.print()
inline fun printd(x: double)	x.print()
inline fun printn(i: int)		i.print()