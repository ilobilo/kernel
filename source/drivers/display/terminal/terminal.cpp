// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/audio/pcspk/pcspk.hpp>
#include <drivers/ps2/ps2.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::audio;

namespace kernel::drivers::display::terminal {

static point pos = { 0, 0 };
new_lock(term_lock);
char *resetcolour = "\033[0m"_c;

limine_terminal **terminals;
limine_terminal *main_term;
uint64_t term_count;

void init()
{
    terminals = terminal_request.response->terminals;
    main_term = terminals[0];
    term_count = terminal_request.response->terminal_count;
}

#pragma region Print
void print(const char *str, limine_terminal *term)
{
    lockit(term_lock);
    if (terminal_request.response == nullptr || term == nullptr) return;
    terminal_request.response->write(term, str, strlen(str));
}

void printi(int num, limine_terminal *term)
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
        while (--i >= 0) printc(temp[i], term);
    }
    else printc('0', term);
}

void printc(char c, limine_terminal *term)
{
    char str[] = { c, 0 };
    print(str, term);
}
#pragma endregion Print

#pragma region Clear
void reset(limine_terminal *term)
{
    lockit(term_lock);
    if (terminal_request.response == nullptr || term == nullptr) return;
    terminal_request.response->write(term, "", LIMINE_TERMINAL_FULL_REFRESH);
}

void clear(const char *ansii_colour, limine_terminal *term)
{
    print(ansii_colour, term);
    print("\033[H\033[2J", term);
}
#pragma endregion Clear

#pragma region CursorCtrl
void cursor_up(int lines, limine_terminal *term)
{
    printf(term, "\033[%dA", lines);
}
void cursor_down(int lines, limine_terminal *term)
{
    printf(term, "\033[%dB", lines);
}
void cursor_right(int lines, limine_terminal *term)
{
    printf(term, "\033[%dC", lines);
}
void cursor_left(int lines, limine_terminal *term)
{
    printf(term, "\033[%dD", lines);
}
#pragma endregion CursorCtrl

#pragma region Misc
void center(const char *text, limine_terminal *term)
{
    for (uint64_t i = 0; i < term->columns / 2 - strlen(text) / 2; i++) printc(' ');
    print(text);
    for (uint64_t i = 0; i < term->columns / 2 - strlen(text) / 2; i++) printc(' ');
}

void callback(limine_terminal *term, uint64_t type, uint64_t first, uint64_t second, uint64_t third)
{
    switch (type)
    {
        case LIMINE_TERMINAL_CB_BELL:
            pcspk::beep(800, 200);
            break;
        case LIMINE_TERMINAL_CB_POS_REPORT:
            pos.X = first;
            pos.Y = second;
            break;
        case LIMINE_TERMINAL_CB_KBD_LEDS:
            switch (first)
            {
                case 0:
                    ps2::kbd_mod.scrolllock = false;
                    ps2::kbd_mod.numlock = false;
                    ps2::kbd_mod.capslock = false;
                    break;
                case 1:
                    ps2::kbd_mod.scrolllock = true;
                    break;
                case 2:
                    ps2::kbd_mod.numlock = true;
                    break;
                case 3:
                    ps2::kbd_mod.capslock = true;
                    break;
            }
            ps2::update_leds();
            break;
    }
}

point getpos(limine_terminal *term)
{
    print("\033[6n", term);
    return pos;
}

#pragma endregion Misc
}

void putchar_(char character)
{
    kernel::drivers::display::terminal::printc(character);
}