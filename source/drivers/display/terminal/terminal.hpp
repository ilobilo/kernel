// Copyright (C) 2021-2022  ilobilo

#pragma once

#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1
#include <printf/printf.h>
#include <lib/math.hpp>
#include <limine.h>
#include <cstdint>
#include <cstdarg>

static inline int printf(limine_terminal *term, const char *format, ...);

namespace kernel::drivers::display::terminal {

extern bool initialised;
extern char *resetcolour;

extern limine_terminal **terminals;
extern limine_terminal *main_term;
extern uint64_t term_count;

void print(const char *string, limine_terminal *term = main_term);
void printi(int i, limine_terminal *term = main_term);
void printc(char c, limine_terminal *term = main_term);

void init();

void cursor_up(int lines = 1, limine_terminal *term = main_term), cursor_down(int lines = 1, limine_terminal *term = main_term), cursor_right(int lines = 1, limine_terminal *term = main_term), cursor_left(int lines = 1, limine_terminal *term = main_term);

void reset(limine_terminal *term = main_term);
void clear(const char *ansii_colour = "\033[0m", limine_terminal *term = main_term);
void center(const char *text, limine_terminal *term = main_term);

void callback(limine_terminal *term, uint64_t type, uint64_t first, uint64_t second, uint64_t third);
point getpos(limine_terminal *term = main_term);

static void check(const char *message, auto init, int64_t args, bool &ok, bool shouldinit = true, limine_terminal *term = main_term)
{
    printf(term, "\033[1m[\033[21m*\033[0m\033[1m]\033[21m %s", message);

    if (shouldinit) reinterpret_cast<void (*)(uint64_t)>(init)(args);

    printf(term, "\033[2G\033[%s\033[0m\033[%dG\033[1m[\033[21m \033[%s\033[0m \033[1m]\033[21m", (ok ? "32m*" : "31m*"), term->columns - 5, (ok ? "32mOK" : "31m!!"));
}
}

static inline int vprintf(limine_terminal *term, const char *fmt, va_list args)
{
    auto printc = reinterpret_cast<void (*)(char, void*)>(kernel::drivers::display::terminal::printc);
    int ret = vfctprintf(printc, term, fmt, args);

    return ret;
}

static inline int printf(limine_terminal *term, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(term, fmt, args);
    va_end(args);

    return ret;
}