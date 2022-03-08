/// --- FYLANG STD --- ///
/// this exists apart from the main compiler.
/// provides some convenience functions
/// that cannot be implemented in fy yet.

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#include <stdio.h>

extern "C" {
/// printd - takes a double and prints it to stderr
DLLEXPORT int eputd(double x) { return fprintf(stderr, "%f", x); }
/// eputn - takes a i32 and prints it to stderr
DLLEXPORT int eputn(int i) { return fprintf(stderr, "%d", i); }
}