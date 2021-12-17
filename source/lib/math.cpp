// Copyright (C) 2021  ilobilo

#include <stdint.h>

static unsigned long next = 1;

int pow(int base, int exp)
{
    int result = 1;
    for (exp = exp; exp > 0; exp--) result *= base;
    return result;
}

int abs(int num)
{
    return num < 0 ? -num : num;
}

int sign(int num)
{
    return (num > 0) ? 1 : ((num < 0) ? -1 : 0);
}

uint64_t rand()
{
    next = next * 1103515245 + 12345;
    return next / 65536 % 32768;
}

void srand(uint64_t seed)
{
    next = seed;
}

uint64_t oct2dec(int oct)
{
    int dec = 0, temp = 0;
    while (oct != 0)
    {
        dec = dec + (oct % 10) * pow(8, temp);
        temp++;
        oct = oct / 10;
    }
    return dec;
}

int intlen(int n)
{
    int digits = 0;
    if (n <= 0)
    {
        n = -n;
        ++digits;
    }
    while (n)
    {
        n /= 10;
        ++digits;
    }
    return digits;
}