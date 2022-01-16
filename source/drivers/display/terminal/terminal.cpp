// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/ps2/keyboard/keyboard.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <stivale2.h>

namespace kernel::drivers::display::terminal {

DEFINE_LOCK(term_lock)

uint16_t columns;
uint16_t rows;

char *colour = (char*)"\033[0m";

void (*write)(const char *, uint64_t);

void init()
{
    log("Initialising terminal\n");

    write = reinterpret_cast<void (*)(const char *, uint64_t)>(term_tag->term_write);
    columns = term_tag->cols;
    rows = term_tag->rows;
}

#pragma region Print
void print(const char *string)
{
    term_lock.lock();
    write(string, strlen(string));
    term_lock.unlock();
}

void printi(int num)
{
    if (num != 0)
    {
        char temp[10];
        int i = 0;
        if (num < 0)
        {
            printc('-');
            num = -num;
        }
        if (num <= 0)
        {
            temp[i++] = '8';
            num = -(num / 10);
        }
        while (num > 0)
        {
            temp[i++] = num % 10 + '0';
            num /= 10;
        }
        while (--i >= 0) printc(temp[i]);
    }
    else printc('0');
}

void printc(char c)
{
    print(char2str(c));
}
#pragma endregion Print

#pragma region Colour
void setcolour(const char *ascii_colour)
{
    colour = const_cast<char*>(ascii_colour);
    printf("%s", colour);
}

void resetcolour()
{
    colour = const_cast<char*>("\033[0m");
    printf("%s", colour);
}
#pragma endregion Colour

#pragma region Clear
void reset()
{
    term_lock.lock();
    write("", STIVALE2_TERM_FULL_REFRESH);
    term_lock.unlock();
}

void clear(const char *ansii_colour)
{
    ps2::kbd::clearbuff();
    setcolour(ansii_colour);
    printf("\033[H\033[2J");
    reset();
}
#pragma endregion Clear

#pragma region CursorCtrl
void cursor_up(int lines)
{
    printf("\033[%dA", lines);
}
void cursor_down(int lines)
{
    printf("\033[%dB", lines);
}
void cursor_right(int lines)
{
    printf("\033[%dC", lines);
}
void cursor_left(int lines)
{
    printf("\033[%dD", lines);
}
#pragma endregion CursorCtrl

#pragma region Misc
void center(const char *text)
{
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++) printc(' ');
    print(text);
    for (uint64_t i = 0; i < columns / 2 - strlen(text) / 2; i++) printc(' ');
}

void check(const char *message)
{
    printf("\033[1m[\033[21m*\033[0m\033[1m]\033[21m %s", message);
    for (uint16_t i = 0; i < columns - strlen(message) - 10; i++) printc(' ');
}

void okerr(bool ok)
{
    if (ok) printf("\033[1m[\033[21m \033[32mOK\033[0m \033[1m]\033[21m");
    else printf("\033[1m[\033[21m \033[31m!!\033[0m \033[1m]\033[21m");
}
#pragma endregion Misc
}

void putchar_(char character)
{
    kernel::drivers::display::terminal::printc(character);
}