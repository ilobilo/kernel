// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/display/terminal/terminal.hpp>
#include <lib/math.hpp>

namespace kernel::drivers::display::font {

extern uint64_t bgcolour;
extern uint64_t fgcolour;
extern point pos;

void setcolour(uint64_t bg = bgcolour, uint64_t fg = fgcolour);

void setpos(uint64_t x, uint64_t y);
void setppos(uint64_t x, uint64_t y);

void printf(const char *fmt, ...);
void printfat(uint64_t x, uint64_t y, const char *fmt, ...);

void pprintf(const char *fmt, ...);
void pprintfat(uint64_t x, uint64_t y, const char *fmt, ...);

void init();
}