#pragma once

extern uint16_t framebuffer_width;
extern uint16_t framebuffer_height;

void term_print(const char *string);

void term_init(struct stivale2_struct_tag_terminal *term_str_tag, struct stivale2_struct_tag_framebuffer *frm);

void cursor_up(int lines), cursor_down(int lines), cursor_right(int lines), cursor_left(int lines);

void term_clear(char *ansii_colour), term_clear();

void term_setcolour(char *ascii_colour), term_resetcolour();