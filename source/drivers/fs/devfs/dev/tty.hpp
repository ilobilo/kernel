// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/display/terminal/terminal.hpp>
#include <lib/pty.hpp>
#include <limine.h>

using namespace kernel::drivers;
using namespace kernel::system;

namespace kernel::drivers::fs::dev::tty {

struct tty_res : pty_res
{
    limine_terminal *thisterm;

    tty_res(limine_terminal *_thisterm) : pty_res(_thisterm->rows, _thisterm->columns), thisterm(_thisterm) { }

    int print(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int ret = vprintf(this->thisterm, fmt, args);
        va_end(args);

        return ret;
    }
};

extern bool initialised;
extern tty_res *current_tty;

void init();
}