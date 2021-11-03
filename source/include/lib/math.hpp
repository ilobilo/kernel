#pragma once

#include <stdint.h>

struct point
{
    long X;
    long Y;
};

uint64_t rand();

int pow(int base, int exponent);

int abs(int num);

int sign(int num);