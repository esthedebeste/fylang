include "types.fy"
inline fun len(a: generic E[generic Len] | *generic E[generic Len]): uint_ptrsize
	Len

// needs to be inline because otherwise the stack just writes over the tempory allocation
inline fun temp_c_str(str: char[generic Len]): *char {
	let alloc = (str, '\0')
	;(&alloc) as *char
}