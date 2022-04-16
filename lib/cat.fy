include "std/io"
include "std/consts"

fun main(argc: int, argv: *char[]) {
	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0])
		return 1
	}

	const filename = argv[1]
	const file = fopen(filename, "r")
	if (file == NULLPTR) {
		printf("Could not open file %s\n", filename)
		return 1
	}

	const BUF_SIZE = 4096
	const buffer = malloc(BUF_SIZE)
	let read = fread(buffer, 1, BUF_SIZE, file)
	while (read != 0) {
		fwrite(buffer, 1, read, stdout)
		read = fread(buffer, 1, BUF_SIZE, file)
	}
	fclose(file)
	0
}