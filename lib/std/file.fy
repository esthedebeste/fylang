include "c/stdio"
include "consts.fy"
include "utils.fy"

type File = *FILE

inline fun open_file(path: char[generic Len] | *char, mode: char[generic Len] | *char): { err: bool, file: File } {
	// convert char[Len] to *char for fopen
	const ppath = if(typeof(path) == *char) path else temp_c_str(path)
	const pmode = if(typeof(mode) == *char) mode else temp_c_str(mode)
	const file: File = fopen(path, mode)
	return (/*err: */file == nullptr, /*file: */file)
}

inline fun(File) __free__()
	fclose(this)

// Returns amount read
inline fun(File) read_into(buffer: *generic Elem[generic Length])
	fread(buffer, sizeof Elem, Length, this)

// Returns amount written
inline fun(File) write_buffer(buffer: *generic Elem[generic Length] | *generic Elem, amount: size_t)
	fwrite(buffer, sizeof Elem, amount, this)

inline fun(File) write(x: generic X)
	x.print_to(this)