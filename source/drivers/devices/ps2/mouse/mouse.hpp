#pragma once

#include <include/math.hpp>
#include <stdint.h>

#define PS2_X_SIGN 0b00010000
#define PS2_Y_SIGN 0b00100000
#define PS2_X_OVER 0b01000000
#define PS2_Y_OVER 0b10000000

enum mousestate
{
    none = 0b00000000,
    left = 0b00000001,
    middle = 0b00000100,
    right = 0b00000010,
};

extern point mousepos;

void Mouse_init();