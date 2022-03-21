// Copyright (C) 2021-2022  ilobilo

#include <system/sched/rtc/rtc.hpp>
#include <lib/math.hpp>
#include <cstdint>

using namespace kernel::system::sched;

static unsigned long next = 1;

int pow(int base, int exp)
{
    int result = 1;
    for (; exp > 0; exp--) result *= base;
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

uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}

uint64_t rand()
{
    next = next * 1103515245 + 12345;
    return next / 65536 % RAND_MAX;
}

void srand(uint64_t seed)
{
    next = seed;
}

[[gnu::constructor]] void runsrand()
{
    srand((rtc::epoch() / rtc::time() - rtc::second() + rtc::minute()) % RAND_MAX - rtc::hour());
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