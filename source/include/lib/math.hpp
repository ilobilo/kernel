#pragma once

namespace kernel::lib::math {

struct point
{
    long X;
    long Y;
};

int pow(int base, int exponent);

int abs(int num);

int sign(int num);
}