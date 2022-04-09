// stdlib.h
include "stddef.fy"

// start and termination
declare fun abort(): void
type _exit_handler = *fun(): void
declare fun atexit(func: _exit_handler): int
declare fun at_quick_exit(func: _exit_handler): int
declare fun exit(status: int): void
declare fun quick_exit(status: int): void
declare fun _Exit(status: int): void

declare fun getenv(name: *char): *char
declare fun system(string: *char): int
// C library memory allocation
declare fun aligned_alloc(alignment: size_t, size: size_t): *void
declare fun calloc(nmemb: size_t, size: size_t): *void
declare fun free(ptr: *void): void
declare fun malloc(size: size_t): *void
declare fun realloc(ptr: *void, size: size_t): *void
declare fun reallocarray(ptr: *void, nmemb: size_t, size: size_t): *void

declare fun atof(nptr: *char): double
declare fun atoi(nptr: *char): int
declare fun atol(nptr: *char): _long_int
declare fun atoll(nptr: *char): _long_long_int
declare fun strtod(nptr: *char, endptr: **char): double
declare fun strtof(nptr: *char, endptr: **char): float
declare fun strtold(nptr: *char, endptr: **char): _long_double
declare fun strtol(nptr: *char, endptr: **char, base: int): _long_int
declare fun strtoll(nptr: *char, endptr: **char, base: int): _long_long_int
declare fun strtoul(nptr: *char, endptr: **char, base: int): unsigned _long_int
declare fun strtoull(nptr: *char, endptr: **char, base: int): unsigned _long_long_int

type _wchar_t = int16
// multibyte / wide string and character conversion functions
declare fun mblen(s: *char, n: size_t): int
declare fun mbtowc(pwc: *_wchar_t, s: *char, n: size_t): int
declare fun wctomb(s: *char, wchar: _wchar_t): int
declare fun mbstowcs(pwcs: *_wchar_t, s: *char, n: size_t): size_t
declare fun wcstombs(s: *char, pwcs: *_wchar_t, n: size_t): size_t

type _compare_pred = *fun(*void, *void): int
// C standard library algorithms
declare fun bsearch(key: *void, base: *void, nmemb: size_t, size: size_t, compar: _compare_pred): *void
declare fun qsort(base: *void, nmemb: size_t, size: size_t, compar: _compare_pred): void

// low-quality random number generation
declare fun rand(): int
declare fun srand(seed: unsigned int): void

// absolute values
declare fun abs(j: int): int
/* // TODO: overrides
	declare fun abs(j: _long_int): _long_int
	declare fun abs(j: _long_long_int): _long_long_int
	declare fun abs(j: float): float
	declare fun abs(j: double): double
	declare fun abs(j: _long_double): _long_double
*/

declare fun labs(j: _long_int): _long_int
declare fun llabs(j: _long_long_int): _long_long_int