include "c/stdio"
include "consts.fy"

type File = *FILE

fun open_file(path: char[], mode: char[]): { err: bool, file: File } {
	const file: File = fopen(path, mode)
	if (file == NULLPTR)
		(true, file)
	else
		(false, file)
}

fun(File) close()
	fclose(this)

// Returns amount read
fun(File) read_into(buffer: *generic Elem[generic Length])
	fread(buffer, sizeof Elem, Length, this)

// Returns amount written
fun(File) write_buffer(buffer: *generic I, amount: size_t)
	if(type I == Elem[generic Length])
		fwrite(buffer, sizeof Elem, amount, this)
	else
		fwrite(buffer, sizeof I, amount, this)

fun(File) write(x: generic X)
	x.print_to(this)