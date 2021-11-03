#include <stdint.h>

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
    static uint64_t x = 123456789;
    static uint64_t y = 362436069;
    static uint64_t z = 521288629;
    static uint64_t w = 88675123;

    uint64_t t;
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return abs(w = w ^ (w >> 19) ^ t ^ (t >> 8));
}