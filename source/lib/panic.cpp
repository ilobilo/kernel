// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <stddef.h>

using namespace kernel::drivers::display;

[[noreturn]] void panic(const char *message, const char *file, size_t line)
{
    serial::err("%s", message);
    serial::err("File: %s", file);
    serial::err("Line: %zu", line);
    serial::err("System halted!\n");

    printf("\n[\033[31mPANIC\033[0m] %s", message);
    printf("\n[\033[31mPANIC\033[0m] File: %s", file);
    printf("\n[\033[31mPANIC\033[0m] Line: %zu", line);
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    while (true) asm volatile ("cli; hlt");
}