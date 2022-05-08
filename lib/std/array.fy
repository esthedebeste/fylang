include "types.fy"
include "consts.fy"
include "c/stdlib"

struct Array<T> {
	ptr: *T,
	length: uint_ptrsize,
	allocated: uint_ptrsize
}

fun create_array(first_elem: generic T): *Array<T> {
	const arr = new Array<T> { ptr = malloc(sizeof T), length = 1, allocated = 1 }
	arr.ptr[0] = first_elem
	arr
}

fun(*Array<generic T>) init() {
	this.ptr = malloc(sizeof T)
	this.length = 0
	this.allocated = 1
}

inline fun(Array<generic T>) __free__()
	free(this.ptr)

// returns the array's new length
fun(*Array<generic T>) push(added: T): uint_ptrsize {
	if(this.length >= this.allocated) {
		this.allocated *= 2
		this.ptr = realloc(this.ptr, this.allocated * sizeof T) as *T
	}
	this.ptr[this.length] = added
	this.length += 1
}


fun(*Array<generic T>) at_ptr(index: int_ptrsize): *T {
	const i: int_ptrsize = if(index < 0) this.length as int_ptrsize + index else index
	if(i < 0 || i >= this.length) null as *T
	else &this.ptr[i]
}

inline fun(*Array<generic T>) set(index: int_ptrsize, to: T): T
	*this.at_ptr(index) = to

fun(*Array<generic T>) at(index: int_ptrsize): T {
	const ptr = this.at_ptr(index)
	if(ptr == (nullptr as *T)) null as T
	else *ptr
}

fun(*Array<generic T>) map(func: *fun(T, uint_ptrsize): T): *Array<T> {
	const arr = new Array<T> { ptr = malloc(this.length * sizeof T), length = this.length, allocated = this.length }
	for(let i = 0; i < this.length; i += 1)
		arr.ptr[i] = func(this.ptr[i], i)
	arr
}

fun(*Array<generic T>) filter(predicate: *fun(T, uint_ptrsize): bool): *Array<T> {
	const arr = new Array<T> { ptr = malloc(this.length * sizeof T), length = 0, allocated = this.length }
	for(let i = 0; i < this.length; i += 1)
		if(predicate(this.ptr[i], i))
			arr.push(this.ptr[i])
	arr
}