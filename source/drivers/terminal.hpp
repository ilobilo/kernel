#pragma once

extern uint16_t columns;
extern uint16_t rows;

extern char *term_colour;

void term_print(const char *string);

void term_init(struct stivale2_struct_tag_terminal *term_str_tag);

void cursor_up(int lines), cursor_down(int lines), cursor_right(int lines), cursor_left(int lines);

void term_clear(char *ansii_colour), term_clear();

void term_setcolour(char *ascii_colour), term_resetcolour();

void term_center(char *text);

void term_check(bool ok, char *mesaage);
