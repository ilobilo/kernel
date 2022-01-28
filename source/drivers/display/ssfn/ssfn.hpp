// Copyright (C) 2021-2022  ilobilo

#pragma once

namespace kernel::drivers::display::ssfn {

extern uint64_t bgcolour;
extern uint64_t fgcolour;
extern point pos;

void setcolour(uint64_t fg = fgcolour, uint64_t bg = bgcolour);

void setpos(uint64_t x, uint64_t y);
void setppos(uint64_t x, uint64_t y);

void printf(const char *fmt, ...);
void printfat(uint64_t x, uint64_t y, const char *fmt, ...);

void pprintf(const char *fmt, ...);
void pprintfat(uint64_t x, uint64_t y, const char *fmt, ...);

void init();
}