#pragma once

#include <stdint.h>

struct point
{
    long X;
    long Y;
};

int pow(int base, int exponent);

int abs(int num);

int sign(int num);

uint64_t rand();
void srand(uint64_t seed);