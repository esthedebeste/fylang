// math.h
include "stddef.fy"

declare fun acos(x: double): double
declare fun asin(x: double): double
declare fun atan(x: double): double
declare fun atan2(y: double, x: double): double
declare fun ceil(x: double): double
declare fun cos(x: double): double
declare fun cosh(x: double): double
declare fun exp(x: double): double
declare fun fabs(x: double): double
declare fun floor(x: double): double
declare fun fmod(x: double, y: double): double
declare fun frexp(value: double, exponent: *int): double
declare fun ldexp(x: double, exp: int): double
declare fun log(x: double): double
declare fun log10(x: double): double
declare fun modf(value: double, iptr: *double): double
declare fun pow(x: double, y: double): double
declare fun sin(x: double): double
declare fun sinh(x: double): double
declare fun sqrt(x: double): double
declare fun tan(x: double): double
declare fun tanh(x: double): double
declare fun erf(x: double): double
declare fun erfc(x: double): double
declare fun gamma(x: double): double
declare fun hypot(x: double, y: double): double
declare fun lgamma(x: double): double
declare fun isnan(x: double): int
declare fun acosh(x: double): double
declare fun asinh(x: double): double
declare fun atanh(x: double): double
declare fun cbrt(x: double): double
declare fun expm1(x: double): double
declare fun ilogb(x: double): int
declare fun log1p(x: double): double
declare fun logb(x: double): double
declare fun nextafter(x: double, y: double): double
declare fun remainder(x: double, y: double): double
declare fun rint(x: double): double