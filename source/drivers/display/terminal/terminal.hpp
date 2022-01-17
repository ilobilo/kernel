// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/display/terminal/printf.h>
#include <stdint.h>
#include <stdarg.h>

namespace kernel::drivers::display::terminal {

#define STIVALE2_TERM_CTX_SIZE ((uint64_t)(-1))
#define STIVALE2_TERM_CTX_SAVE ((uint64_t)(-2))
#define STIVALE2_TERM_CTX_RESTORE ((uint64_t)(-3))
#define STIVALE2_TERM_FULL_REFRESH ((uint64_t)(-4))

extern uint16_t columns;
extern uint16_t rows;

extern char *colour;

void print(const char *string);
void printi(int i);
void printc(char c);

void init();

void cursor_up(int lines = 1), cursor_down(int lines = 1), cursor_right(int lines = 1), cursor_left(int lines = 1);

void clear(const char *ansii_colour = colour);
void setcolour(const char *ascii_colour), resetcolour();
void center(const char *text);

void check(const char *message, uint64_t init, int64_t args, bool &ok, bool shouldinit = true);

//void printf(char *c, ...);
}