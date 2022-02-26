/// --- FYLANG STD --- ///
/// this exists apart from the main compiler.
/// provides some convenience functions.

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#include <stdio.h>

extern "C"
{
    /// putchard - putchar that takes a double and returns it as a char.
    DLLEXPORT char putchard(double X)
    {
        fputc((char)X, stderr);
        return X;
    }

    /// printd - printf that takes a double prints it as "%f\n", returning 0.
    DLLEXPORT double printd(double X)
    {
        fprintf(stderr, "%f\n", X);
        return 0;
    }

    /// int_floor - takes a double and returns it in int format (floored)
    DLLEXPORT int int_floor(double X)
    {
        return X;
    }

    /// int_floor - takes a c-string and prints it to stderr (without newline)
    DLLEXPORT int eputs(char *str)
    {
        return fputs(str, stderr);
    }
}