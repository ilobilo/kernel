// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::drivers::ps2::kbd {

struct kbd_mod_t
{
    int shift : 1;
    int ctrl : 1;
    int alt : 1;
    int numlock : 1;
    int capslock : 1;
    int scrolllock : 1;
};

extern bool initialised;
extern char *buff;

void clearbuff();

char getchar();
char *getline();

void init();
}