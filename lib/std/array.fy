include "types.fy"
include "c/stdlib"

const PTR_SIZE = sizeof *unknown
struct Array {
	ptr: **unknown, // TODO: generics/templates to type this pointer
	length: uint_ptrsize,
	allocated: uint_ptrsize
}

fun create_array(): *Array
	new Array { ptr = calloc(1, PTR_SIZE), length = 0, allocated = 1 }

// returns the array's new length
fun(*Array) push(added: *unknown): uint_ptrsize {
	if(this.length >= this.allocated) {
		this.allocated *= 2
		reallocarray(this.ptr, this.allocated, PTR_SIZE)
	}
	this.ptr[this.length] = added
	this.length += 1
}

fun(*Array) set(index: uint_ptrsize, to: *unknown): *unknown
	this.ptr[index] = to

fun(*Array) at(index: int_ptrsize): *unknown
	if(index < 0) 
		this.ptr[this.length + index]
	else
		this.ptr[index]

fun(*Array) map(func: *fun(*unknown, uint_ptrsize): *unknown): *Array {
	const arr = new Array { ptr = calloc(this.length, PTR_SIZE), length = this.length, allocated = this.length }
	for(let i = 0; i < this.length; i += 1)
		arr.ptr[i] = func(this.ptr[i], i)
	arr
}

fun(*Array) filter(predicate: *fun(*unknown, uint_ptrsize): bool): *Array {
	const arr = new Array { ptr = calloc(this.length, PTR_SIZE), length = 0, allocated = this.length }
	for(let i = 0; i < this.length; i += 1)
		if(predicate(this.ptr[i], i))
			arr.push(this.ptr[i])
	arr
}