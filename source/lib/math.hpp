// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

#define RAND_MAX 32768

#define DIV_ROUNDUP(A, B) \
({ \
    typeof(A) _a_ = A; \
    typeof(B) _b_ = B; \
    (_a_ + (_b_ - 1)) / _b_; \
})

#define ALIGN_UP(A, B) \
({ \
    typeof(A) _a__ = A; \
    typeof(B) _b__ = B; \
    DIV_ROUNDUP(_a__, _b__) * _b__; \
})

#define ALIGN_DOWN(A, B) \
({ \
    typeof(A) _a_ = A; \
    typeof(B) _b_ = B; \
    (_a_ / _b_) * _b_; \
})

#define POWER_OF_2(var) (!((var) & ((var) - 1)))

#define TO_POWER_OF_2(var) \
({ \
    typeof(var) i = (var) - 1; \
    while (i & i - 1) i = i & i - 1; \
    i <<= 1; \
})

#define ALIGN_UP_2(align, var) \
({ \
    typeof(var) a = (var); \
    while (a % (align) || !POWER_OF_2(a)) \
    { \
        a++; \
        a = TO_POWER_OF_2(a); \
    } \
    a; \
})

struct point
{
    size_t X = 0;
    size_t Y = 0;
};

int pow(int base, int exponent);
int abs(int num);
int sign(int num);

uint64_t jdn(uint8_t days, uint8_t months, uint16_t years);

uint64_t rand();
void srand(uint64_t seed);

uint64_t oct2dec(int n);
int intlen(int n);