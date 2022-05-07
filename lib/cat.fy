include "std/io"
include "std/file"

fun main(argc: int, argv: **char) {
	if (argc != 2) {
		print("Usage: ") print(argv[0]) print(" <file>\n")
		return 1
	}

	const filename = argv[1]
	const res = open_file(filename, "r"c)
	if (res.err) {
		print("Could not open file ") print(filename)
		return 1
	}
	const file = res.file

	let buffer: char[512]
	let amount_read = file.read_into(&buffer)
	while (amount_read != 0) {
		stdout.write_buffer(&buffer, amount_read)
		amount_read = file.read_into(&buffer)
	}
	file.close()
	0
}