// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <lib/log.hpp>
#include <cstddef>
#include <utility>

namespace kernel::system::sched::scheduler
{
    void kill();
}

[[noreturn]] void panic(const char *message, const char *func, std::source_location location)
{
    error("PANIC: %s", message);
    error("File: %s", location.file());
    error("Function: %s", func);
    error("Line: %u", location.line());
    error("Column: %u", location.column());
    error("System halted!\n");

    printf("\n[\033[31mPANIC\033[0m] %s", message);
    printf("\n[\033[31mPANIC\033[0m] File: %s", location.file());
    printf("\n[\033[31mPANIC\033[0m] Function: %s", func);
    printf("\n[\033[31mPANIC\033[0m] Line: %u", location.line());
    printf("\n[\033[31mPANIC\033[0m] Column: %u", location.column());
    printf("\n[\033[31mPANIC\033[0m] System halted!\n");

    kernel::system::sched::scheduler::kill();
    while (true) asm volatile ("cli; hlt");
}