#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mutex/mutex.hpp>
#include <stivale2.h>
#include <main.hpp>
#include <lib/string.hpp>

uint16_t columns;
uint16_t rows;

char *term_colour = "\033[0m";

void (*term_write)(const char *string, uint64_t length);

void term_init()
{
    serial_info("Initialising terminal\n");

    void *term_write_ptr = (void*)term_tag->term_write;
    term_write = (void (*)(const char *string, uint64_t length))term_write_ptr;
    columns = term_tag->cols;
    rows = term_tag->rows;
}

#pragma region Print
DEFINE_MUTEX(m_term_write);
void term_print(const char *string)
{
    mutex_lock(&m_term_write);
    term_write(string, strlen(string));
    mutex_unlock(&m_term_write);
}

void term_printi(int num)
{
    if (num != 0)
    {
        char temp[10];
        int i = 0;
        if (num < 0)
        {
            term_print("-");
            num = -num;
        }
        if (num > 0); else { temp[i++] = '8'; num = -(num / 10); }
        while (num > 0)
        {
            temp[i++] = num % 10 + '0';
            num /= 10;
        }
        while (--i >= 0)
        {
            term_printc(temp[i]);
        }
    }
    else
    {
        term_print("0");
    }
}

void term_printc(char c)
{
    char cs[2] = "\0";
    cs[0] = c;
    term_print(cs);
}

void _putchar(char character)
{
    term_printc(character);
}
#pragma endregion Print

#pragma region Colour
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
#pragma endregion Colour

#pragma region Clear
void term_reset()
{
    term_write("", STIVALE2_TERM_FULL_REFRESH);
}

void term_clear(char *ansii_colour)
{
    clearbuff();
    term_setcolour(ansii_colour);
    term_print("\033[H\033[2J");
    term_reset();
}
#pragma endregion Clear

#pragma region CursorCtrl
void cursor_up(int lines = 1)
{
    printf("\033[%dA", lines);
}
void cursor_down(int lines = 1)
{
    printf("\033[%dB", lines);
}
void cursor_right(int lines = 1)
{
    printf("\033[%dC", lines);
}
void cursor_left(int lines = 1)
{
    printf("\033[%dD", lines);
}
#pragma endregion CursorCtrl

#pragma region Misc
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
    if (ok)
    {
        term_print("\033[1m[\033[21m\033[32m*\033[0m\033[1m]\033[21m ");
        term_print(message);
        for (uint16_t i = 0; i < columns - (10 + strlen(message)); i++)
        {
            term_print(" ");
        }
        term_print("\033[1m[\033[21m \033[32mOK\033[0m \033[1m]\033[21m");
    }
    else
    {
        term_print("\033[1m[\033[21m\033[31m*\033[0m\033[1m]\033[21m ");
        term_print(message);
        for (uint16_t i = 0; i < columns - (10 + strlen(message)); i++)
        {
            term_print(" ");
        }
        term_print("\033[1m[\033[21m \033[31m!!\033[0m \033[1m]\033[21m");
    }
}
#pragma endregion Misc

/* Printf
void printf(char *c, ...)
{
    char *s;
    va_list lst;
    va_start(lst, c);
    while(*c != '\0')
    {
        if(*c != '%')
        {
            term_printc(*c);
            c++;
            continue;
        }

        c++;

        if(*c == '\0')
        {
            break;
        }

        switch(*c)
        {
            case 's': term_print(va_arg(lst, char *)); break;
            case 'c': term_printc(va_arg(lst, int)); break;
            case 'd': case 'i': term_printi(va_arg(lst, int)); break;
        }
        c++;
    }
}
*/