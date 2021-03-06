// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/math.hpp>

namespace kernel::drivers::display::ssfn {

extern uint64_t bgcolour;
extern uint64_t fgcolour;
extern point pos;

void setcolour(uint64_t fg = fgcolour, uint64_t bg = bgcolour);

void setpos(uint64_t x, uint64_t y);
void resetpos();

void printf(const char *fmt, ...);
void printfat(uint64_t x, uint64_t y, const char *fmt, ...);

void init(uint64_t sfn);
}