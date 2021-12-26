// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::drivers::ps2::kbd {

struct kbd_mod_t
{
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;
    bool numlock : 1;
    bool capslock : 1;
    bool scrolllock : 1;
};

extern bool initialised;
extern char *buff;

void clearbuff();

char getchar();
char *getline();

void init();
}