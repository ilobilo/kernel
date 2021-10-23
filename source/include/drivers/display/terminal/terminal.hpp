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

void cursor_up(int lines), cursor_down(int lines), cursor_right(int lines), cursor_left(int lines);

void clear(char *ansii_colour = colour);

void setcolour(char *ascii_colour), term_resetcolour();

void center(char *text);

void check(bool ok, char *mesaage);

//void printf(char *c, ...);
}