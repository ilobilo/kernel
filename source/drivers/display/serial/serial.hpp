// Copyright (C) 2021-2022  ilobilo

#pragma once

namespace kernel::drivers::display::serial {

enum COMS
{
    COM1 = 0x3F8,
    COM2 = 0x2F8
};

extern bool initialised;

void printc(char c, void *arg = nullptr);
void print(const char *fmt, ...);
void newline();

void init();
}