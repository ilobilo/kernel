#include <stddef.h>
#include "../kernel.hpp"
#include "../stivale2.h"
#include "../include/string.hpp"
#include "terminal.hpp"

uint16_t columns;
uint16_t rows;

char *term_colour = "\033[0m";

void (*term_write)(const char *string, int length);

void term_init(struct stivale2_struct_tag_terminal *term_str_tag)
{
    void *term_write_ptr = (void *)term_str_tag->term_write;
    term_write = (void (*)(const char *string, int length))term_write_ptr;
    columns = term_str_tag->cols;
    rows = term_str_tag->rows;

    term_clear();
}

void term_print(const char *string)
{
    term_write(string, strlen(string));
}

void term_printi(int i)
{
    char *out;
    int_to_string(i, out);
    term_print(out);
}

void term_setcolour(char *ascii_colour)
{
    term_colour = ascii_colour;
    term_print(term_colour);
}

void term_resetcolour()
{
    term_colour = "\033[0m";
    term_print(term_colour);
}

void term_clear(char *ansii_colour)
{
    term_print("\033[H");
    for (uint16_t i = 0; i < rows; i++)
    {
        for (uint16_t i = 0; i < columns; i++)
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
    for (uint16_t i = 0; i < columns; i++)
    {
        for (uint16_t i = 0; i < rows; i++)
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

void term_center(char *text)
{
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++)
    {
        term_print(" ");
    }
    term_print(text);
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++)
    {
        term_print(" ");
    }
}

void term_check(bool ok, char *message)
{
    term_print("\033[1m[\033[21m\033[32m*\033[0m\033[1m]\033[21m ");
    term_print(message);
    if (ok)
    {
        for (uint16_t i = 0; i < columns - (10 + strlen(message)); i++)
        {
            term_print(" ");
        }
        term_print("\033[1m[\033[21m \033[32mOK\033[0m \033[1m]\033[21m");
    }
    else
    {
        for (uint16_t i = 0; i < columns - (11 + strlen(message)); i++)
        {
            term_print(" ");
        }
        term_print("\033[1m[\033[21m \033[31mERR\033[0m \033[1m]\033[21m");
    }
}
