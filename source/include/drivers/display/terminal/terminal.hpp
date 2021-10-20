#pragma once

#include <drivers/display/terminal/printf.h>
#include <stdint.h>
#include <stdarg.h>

#define STIVALE2_TERM_CTX_SIZE ((uint64_t)(-1))
#define STIVALE2_TERM_CTX_SAVE ((uint64_t)(-2))
#define STIVALE2_TERM_CTX_RESTORE ((uint64_t)(-3))
#define STIVALE2_TERM_FULL_REFRESH ((uint64_t)(-4))

extern uint16_t columns;
extern uint16_t rows;

extern char *term_colour;

void term_print(const char *string);

void term_printi(int i);

void term_printc(char c);

void term_init();

void cursor_up(int lines), cursor_down(int lines), cursor_right(int lines), cursor_left(int lines);

void term_clear(char *ansii_colour = term_colour);

void term_setcolour(char *ascii_colour), term_resetcolour();

void term_center(char *text);

void term_check(bool ok, char *mesaage);

//void printf(char *c, ...);