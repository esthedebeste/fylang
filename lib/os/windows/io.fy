include "c/stdio"

// _ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned _Ix);
declare fun __acrt_iob_func(_Ix: uint32): *FILE

let stdin  = __acrt_iob_func(0)
let stdout = __acrt_iob_func(1)
let stderr = __acrt_iob_func(2)