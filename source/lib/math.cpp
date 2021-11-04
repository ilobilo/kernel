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
    return (uint64_t)(next / 65536) % 32768;
}

void srand(uint64_t seed)
{
    next = seed;
}