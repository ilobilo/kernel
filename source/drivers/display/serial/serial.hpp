// Copyright (C) 2021  ilobilo

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

void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void err(const char *fmt, ...);

void init();
}