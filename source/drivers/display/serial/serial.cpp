// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/io.hpp>

using namespace kernel::system::cpu;

namespace kernel::drivers::display::serial {

bool initialised = false;
DEFINE_LOCK(lock)

bool check()
{
    if (initialised) return true;
    else return false;
}

int is_transmit_empty()
{
    return inb(COMS::COM1 + 5) & 0x20;
}

int received()
{
    return inb(COMS::COM1 + 5) & 1;
}

char read()
{
    while (!received());
    return inb(COMS::COM1);
}

void printc(char c, __attribute__((unused)) void *arg)
{
    if (!check()) return;
    while (!is_transmit_empty());
    outb(COMS::COM1, c);
}

void serial_printf(const char *fmt, ...)
{
    acquire_lock(&lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);
    release_lock(&lock);
}

void info(const char *fmt, ...)
{
    acquire_lock(&lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, "[INFO] ", args);
    vfctprintf(&printc, nullptr, fmt, args);
    vfctprintf(&printc, nullptr, "\n", args);
    va_end(args);
    release_lock(&lock);
}

void warn(const char *fmt, ...)
{
    acquire_lock(&lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, "[\033[33mWARN\033[0m] ", args);
    vfctprintf(&printc, nullptr, fmt, args);
    vfctprintf(&printc, nullptr, "\n", args);
    va_end(args);
    release_lock(&lock);
}

void err(const char *fmt, ...)
{
    acquire_lock(&lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, "[\033[31mERROR\033[0m] ", args);
    vfctprintf(&printc, nullptr, fmt, args);
    vfctprintf(&printc, nullptr, "\n", args);
    va_end(args);
    release_lock(&lock);
}

void newline()
{
    serial_printf("\n");
}

static void COM1_Handler(idt::registers_t *)
{
    char c = read();
    switch (c)
    {
        case 13:
            c = '\n';
            break;
        case 8:
        case 127:
            printf("\b ");
            serial_printf("\b ");
            c = '\b';
            break;
    }
    printf("%c", c);
    serial_printf("%c", c);
}

void init()
{
    if (initialised) return;

    outb(COMS::COM1 + 1, 0x00);
    outb(COMS::COM1 + 3, 0x80);
    outb(COMS::COM1 + 0, 0x03);
    outb(COMS::COM1 + 1, 0x00);
    outb(COMS::COM1 + 3, 0x03);
    outb(COMS::COM1 + 2, 0xC7);
    outb(COMS::COM1 + 4, 0x0B);

    serial_printf("\033[0m");

    register_interrupt_handler(idt::IRQ4, COM1_Handler);
    outb(COMS::COM1 + 1, 0x01);

    initialised = true;
}
}