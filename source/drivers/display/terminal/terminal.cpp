#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/sched/lock/lock.hpp>
#include <lib/string.hpp>
#include <stivale2.h>
#include <main.hpp>


namespace kernel::drivers::display::terminal {

DEFINE_LOCK(lock);

uint16_t columns;
uint16_t rows;

char *colour = "\033[0m";

void (*write)(const char *string, uint64_t length);

void init()
{
    serial::info("Initialising terminal\n");

    void *write_ptr = (void*)term_tag->term_write;
    write = (void (*)(const char *string, uint64_t length))write_ptr;
    columns = term_tag->cols;
    rows = term_tag->rows;
}

#pragma region Print
void print(const char *string)
{
    acquire_lock(&lock);
    write(string, strlen(string));
    release_lock(&lock);
}

void printi(int num)
{
    if (num != 0)
    {
        char temp[10];
        int i = 0;
        if (num < 0)
        {
            print("-");
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
            printc(temp[i]);
        }
    }
    else
    {
        print("0");
    }
}

void printc(char c)
{
    char cs[2] = "\0";
    cs[0] = c;
    print(cs);
}

#pragma endregion Print

#pragma region Colour
void setcolour(char *ascii_colour)
{
    colour = ascii_colour;
    print(colour);
}

void resetcolour()
{
    colour = "\033[0m";
    print(colour);
}
#pragma endregion Colour

#pragma region Clear
void reset()
{
    acquire_lock(&lock);
    write("", STIVALE2_TERM_FULL_REFRESH);
    release_lock(&lock);
}

void clear(char *ansii_colour)
{
    ps2::kbd::clearbuff();
    setcolour(ansii_colour);
    print("\033[H\033[2J");
    reset();
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
void center(char *text)
{
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++)
    {
        print(" ");
    }
    print(text);
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++)
    {
        print(" ");
    }
}

void check(bool ok, char *message)
{
    if (ok)
    {
        print("\033[1m[\033[21m\033[32m*\033[0m\033[1m]\033[21m ");
        print(message);
        for (uint16_t i = 0; i < columns - (10 + strlen(message)); i++)
        {
            print(" ");
        }
        print("\033[1m[\033[21m \033[32mOK\033[0m \033[1m]\033[21m");
    }
    else
    {
        print("\033[1m[\033[21m\033[31m*\033[0m\033[1m]\033[21m ");
        print(message);
        for (uint16_t i = 0; i < columns - (10 + strlen(message)); i++)
        {
            print(" ");
        }
        print("\033[1m[\033[21m \033[31m!!\033[0m \033[1m]\033[21m");
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
            printc(*c);
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
            case 's': print(va_arg(lst, char *)); break;
            case 'c': printc(va_arg(lst, int)); break;
            case 'd': case 'i': printi(va_arg(lst, int)); break;
        }
        c++;
    }
}
*/
}

void _putchar(char character)
{
    kernel::drivers::display::terminal::printc(character);
}