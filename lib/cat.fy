include "std/io"
include "std/consts"

fun main(argc: int, argv: *char[]) {
	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0])
		return 1
	}

	const out = stdout()
	const filename = argv[1]
	const file = fopen(filename, "r")
	if (file == NULLPTR) {
		printf("Could not open file %s\n", filename)
		return 1
	}

	let buffer: char[512]
	let read = fread(&buffer, 1, 512, file)
	while (read != 0) {
		fwrite(&buffer, 1, read, out)
		read = fread(&buffer, 1, 512, file)
	}
	fclose(file)
	0
}