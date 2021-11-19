// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/math.hpp>
#include <stdint.h>

namespace kernel::drivers::ps2::mouse {

#define PS2_X_SIGN 0b00010000
#define PS2_Y_SIGN 0b00100000
#define PS2_X_OVER 0b01000000
#define PS2_Y_OVER 0b10000000

enum mousestate
{
    ps2_none = 0b00000000,
    ps2_left = 0b00000001,
    ps2_middle = 0b00000100,
    ps2_right = 0b00000010,
};

extern bool initialised;
extern bool vmware;

extern point pos;
extern point posold;
extern uint32_t mousebordercol;
extern uint32_t mouseinsidecol;

mousestate getmousestate();

void draw();
void clear();

void init();
}