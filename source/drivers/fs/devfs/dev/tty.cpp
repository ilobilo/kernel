// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/devfs/dev/tty.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <kernel/kernel.hpp>

namespace kernel::drivers::fs::dev::tty {

bool initialised = false;
tty_res *current_tty = nullptr;

void init()
{
    if (initialised) return;

    for (size_t i = 0; i < terminal::term_count; i++)
    {
        tty_res *tty = new tty_res(terminal::terminals[i]);

        std::string ttyname("tty");
        ttyname.push_back(i + '0');
        devfs::add(tty, ttyname);
        if (current_tty == nullptr) current_tty = tty;
    }

    initialised = true;
}
}