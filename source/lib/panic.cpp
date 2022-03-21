// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <lib/log.hpp>
#include <cstddef>
#include <utility>

using namespace kernel::drivers::display;

[[noreturn]] void panic(const char *message, std::source_location location)
{
    error("PANIC: %s", message);
    error("File: %s", location.file());
    error("Function: %s", location.function());
    error("Line: %u", location.line());
    error("Column: %u", location.column());
    error("System halted!\n");

    printf("\n[\033[31mPANIC\033[0m] %s", message);
    printf("\n[\033[31mPANIC\033[0m] File: %s", location.file());
    printf("\n[\033[31mPANIC\033[0m] Function: %s", location.function());
    printf("\n[\033[31mPANIC\033[0m] Line: %u", location.line());
    printf("\n[\033[31mPANIC\033[0m] Column: %u", location.column());
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    while (true) asm volatile ("cli; hlt");
}