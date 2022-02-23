#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma once
static double pow(double a, double to)
{
    double result = a;
    for (int i = 1; i < to; i++)
        a *= a;
    return result;
}
static bool streq(const char *a, const unsigned int alen, const char *b, const unsigned int blen)
{
    if (alen != blen)
        return false;
    for (unsigned int i = 0; i < alen; i++)
        if (a[i] != b[i])
            return false;
    return true;
}
void error(const char *str)
{
    fprintf(stderr, "Error: %s\n", str);
    exit(1);
}