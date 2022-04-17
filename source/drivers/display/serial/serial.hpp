// Copyright (C) 2021-2022  ilobilo

#pragma once

namespace kernel::drivers::display::serial {

enum COMS
{
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8
};

extern bool initialised;

void printc(char c, void *arg = nullptr);
int print(COMS com, const char *fmt, ...);
void newline(COMS com = COM1);

void early_init();
void init();
}