/// --- FYLANG STD --- ///

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#include <stdio.h>

extern "C"
{
    /// putchard - putchar that takes a double and returns 0.
    DLLEXPORT double putchard(double X)
    {
        fputc((char)X, stderr);
        return 0;
    }

    /// printd - printf that takes a double prints it as "%f\n", returning 0.
    DLLEXPORT double printd(double X)
    {
        fprintf(stderr, "%f\n", X);
        return 0;
    }

    /// int_floor - takes a double and returns it in int format (floored)
    /// math won't work anymore, exclusively using this for main's return.
    DLLEXPORT int intfloor(double X)
    {
        return (int)X;
    }
}