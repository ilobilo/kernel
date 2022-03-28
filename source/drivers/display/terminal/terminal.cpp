// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::display::terminal {

char *colour = "\033[0m"_c;
new_lock(term_lock);

#pragma region Print
void print(const char *str)
{
    lockit(term_lock);
    if (terminal_request.response == nullptr) return;
    terminal_request.response->write(str, strlen(str));
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
    char str[] = { c, 0 };
    print(str);
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
    print("\033[0m");
}
#pragma endregion Colour

#pragma region Clear
void reset()
{
    lockit(term_lock);
    if (terminal_request.response == nullptr) return;
    terminal_request.response->write("", LIMINE_TERMINAL_FULL_REFRESH);
}

void clear(const char *ansii_colour)
{
    setcolour(ansii_colour);
    print("\033[H\033[2J");
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
    for (uint64_t i = 0; i < terminal_request.response->columns / 2 - strlen(text) / 2; i++) printc(' ');
    print(text);
    for (uint64_t i = 0; i < terminal_request.response->columns / 2 - strlen(text) / 2; i++) printc(' ');
}
void check(const char *message, uint64_t init, int64_t args, bool &ok, bool shouldinit)
{
    printf("\033[1m[\033[21m*\033[0m\033[1m]\033[21m %s", message);

    if (shouldinit) reinterpret_cast<void (*)(uint64_t)>(init)(args);

    printf("\033[2G\033[%s\033[0m\033[%dG\033[1m[\033[21m \033[%s\033[0m \033[1m]\033[21m", (ok ? "32m*" : "31m*"), terminal_request.response->columns - 5, (ok ? "32mOK" : "31m!!"));
}
#pragma endregion Misc
}

void putchar_(char character)
{
    kernel::drivers::display::terminal::printc(character);
}