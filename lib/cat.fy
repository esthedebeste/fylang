include "std/io"
include "std/file"

fun main(argc: int, argv: *char[]) {
	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0])
		return 1
	}

	const filename = argv[1]
	const res = open_file(filename, "r")
	const err = res.err
	const file = res.file
	if (err) {
		printf("Could not open file %s\n", filename)
		return 1
	}

	let buffer: char[512]
	let amount_read = file.read_into(&buffer)
	while (amount_read != 0) {
		stdout.write_buffer(&buffer, amount_read)
		amount_read = file.read_into(&buffer)
	}
	file.close()
	0
}