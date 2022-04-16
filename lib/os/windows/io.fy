include "c/stdio"

// _ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned _Ix);
declare fun __acrt_iob_func(_Ix: uint32): *FILE

fun stdin():  *FILE __acrt_iob_func(0)
fun stdout(): *FILE __acrt_iob_func(1)
fun stderr(): *FILE __acrt_iob_func(2)