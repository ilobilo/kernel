// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/display/terminal/terminal.hpp>
#include <lib/terminal.hpp>
#include <limine.h>

using namespace kernel::drivers;
using namespace kernel::system;

namespace kernel::drivers::fs::dev::tty {

struct tty_res : terminal_res
{
    limine_terminal *thisterm;

    tty_res(limine_terminal *_thisterm) : terminal_res(_thisterm->rows, _thisterm->columns), thisterm(_thisterm) { }

    int print(const char *fmt, ...)
    {
        limine_terminal *oldterm = terminal::main_term;
        terminal::main_term = this->thisterm;

        va_list args;
        va_start(args, fmt);
        int ret = vprintf(fmt, args);
        va_end(args);

        terminal::main_term = oldterm;
        return ret;
    }
};

extern bool initialised;
extern tty_res *current_tty;

void init();
}