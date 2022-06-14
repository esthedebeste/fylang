include "c/stdio"

declare fun four cc(C)(): int
__asm__(".globl four
		.type four, @function
		four:
		.cfi_startproc
		movl $4, %eax
		ret
		.cfi_endproc")

fun main() {
	let a = four()
	// 		returns int32  copy eax to ebx 	outreg is ebx  load a into eax
	const b = __asm__ int("mov %eax, %ebx" => 	ebx)		(eax = a)
	if(b == 4)	puts("OK"c)
	else { puts("FAIL"c) return 1 }
	let c = 5
	// side effect code, basically does "c = a" in AT&T assembly
	if(type uint_ptrsize == uint64)
		// 64 bit register for the pointer to c
		__asm__("mov %eax, (%rbx)")(eax = a, rbx = &c)
	else if(type uint_ptrsize == uint32)
		// 32 bit register for the pointer to c
		__asm__("mov %eax, (%ebx)")(eax = a, ebx = &c)
	else puts("tests/asm.fy: unknown pointer size"c)
	if(c == 4)	puts("OK"c)
	else { puts("FAIL"c) return 1 }
	0
}