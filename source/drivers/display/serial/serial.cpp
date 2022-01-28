// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/lock.hpp>
#include <lib/io.hpp>

using namespace kernel::system::cpu;

namespace kernel::drivers::display::serial {

bool initialised = false;
DEFINE_LOCK(serial_lock)

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

void printc(char c, void *arg)
{
    if (!check()) return;
    while (!is_transmit_empty());
    outb(COMS::COM1, c);
}

void print(const char *fmt, ...)
{
    serial_lock.lock();
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);
    serial_lock.unlock();
}

void newline()
{
    print("\n");
}

static void COM1_Handler(registers_t *)
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
            print("\b ");
            c = '\b';
            break;
    }
    printf("%c", c);
    print("%c", c);
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

    print("\033[0m");

    idt::register_interrupt_handler(idt::IRQ4, COM1_Handler);
    outb(COMS::COM1 + 1, 0x01);

    initialised = true;
}
}