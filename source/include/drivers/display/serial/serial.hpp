// Copyright (C) 2021  ilobilo

#pragma once

namespace kernel::drivers::display::serial {

enum COMS
{
    COM1 = 0x3F8,
    COM2 = 0x2F8
};

extern bool initialised;

void printc(char c, void *arg);

void serial_printf(const char *fmt, ...);

void info(const char *fmt, ...);

void err(const char *fmt, ...);

void newline();

void init();
}