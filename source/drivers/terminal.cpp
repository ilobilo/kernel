#include <stddef.h>
#include "../kernel.hpp"
#include "../stivale2.h"
#include "../include/string.hpp"

uint16_t framebuffer_width;
uint16_t framebuffer_height;

char *term_colour = "\033[37;40m";

void (*term_write)(const char *string, int length);

void term_init(struct stivale2_struct_tag_terminal *term_str_tag, struct stivale2_struct_tag_framebuffer *frm)
{
    void *term_write_ptr = (void *)term_str_tag->term_write;
    term_write = (void (*)(const char *string, int length))term_write_ptr;
    framebuffer_width = frm->framebuffer_width;
    framebuffer_height = frm->framebuffer_height;
}

void term_print(const char *string)
{
    term_write(string, strlen(string));
}

void term_setcolour(char *ascii_colour)
{
    term_colour = ascii_colour;
    term_print(term_colour);
}

void term_resetcolour()
{
    term_colour = "\033[37;40m";
    term_print(term_colour);
}

void term_clear(char *ansii_colour)
{
    term_print("\033[H");
    for (uint16_t i = 0; i < framebuffer_width; i++)
    {
        for (uint16_t i = 0; i < framebuffer_height; i++)
        {
            term_print(ansii_colour);
            term_print(" ");
        }
    }
    term_print("\033[H");
}

void term_clear()
{
    term_print("\033[H");
    for (uint16_t i = 0; i < framebuffer_width; i++)
    {
        for (uint16_t i = 0; i < framebuffer_height; i++)
        {
            term_print(term_colour);
            term_print(" ");
        }
    }
    term_print("\033[H");
}

void cursor_up(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        term_print("\033[1A");
    }
}
void cursor_down(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        term_print("\033[1B");
    }
}
void cursor_right(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        term_print("\033[1C");
    }
}
void cursor_left(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        term_print("\033[1D");
    }
}