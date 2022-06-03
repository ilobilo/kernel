// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/vector.hpp>
#include <lib/lock.hpp>
#include <lib/pty.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::fs;
using namespace kernel::system::cpu;
using namespace kernel::system;

namespace kernel::drivers::display::serial {

struct ttys_res : pty_res
{
    COMS thiscom;

    ttys_res(COMS _thiscom) : pty_res(0, 0), thiscom(_thiscom) { }

    int print(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int ret = vfctprintf(printc, reinterpret_cast<void*>(this->thiscom), fmt, args);
        va_end(args);

        return ret;
    }
};

bool initialised = false;
ttys_res *current_ttys = nullptr;
new_lock(serial_lock);

static bool is_transmit_empty(COMS com = COM1)
{
    return inb(com + 5) & 0x20;
}

static bool received(COMS com = COM1)
{
    uint8_t status = inb(com + 5);
    return (status != 0xFF) && (status & 1);
}

static char read(COMS com = COM1)
{
    while (!received());
    return inb(com);
}

void printc(char c, void *arg)
{
    while (!is_transmit_empty());
    outb(reinterpret_cast<uintptr_t>(arg), c);
}

int print(COMS com, const char *fmt, ...)
{
    lockit(serial_lock);

    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(&printc, reinterpret_cast<void*>(com), fmt, args);
    va_end(args);

    return ret;
}

void newline(COMS com)
{
    print(com, "\n");
}

static void com1_handler(registers_t *)
{
    char c = read(COM1);
    if (c == 0) return;
    current_ttys->add_char(c);
}

static void com2_handler(registers_t *)
{
    char c = read(COM2);
    if (c == 0) return;
    current_ttys->add_char(c);
}

static void initport(COMS com)
{
    outb(com + 1, 0x00);
    outb(com + 3, 0x80);
    outb(com + 0, 0x01);
    outb(com + 1, 0x00);
    outb(com + 3, 0x03);
    outb(com + 2, 0xC7);
    outb(com + 4, 0x0B);

    print(com, "\033[0m\n");
}

void early_init()
{
    initport(COM1);
    initport(COM2);

    idt::register_interrupt_handler(idt::IRQ4, com1_handler);
    idt::register_interrupt_handler(idt::IRQ3, com2_handler);

    outb(COM1 + 1, 0x01);
    outb(COM2 + 1, 0x01);
}

void init()
{
    if (initialised) return;

    ttys_res *res = new ttys_res(COM1);
    devfs::add(res, "ttyS0");

    current_ttys = res;

    res = new ttys_res(COM2);
    devfs::add(res, "ttyS1");

    initialised = true;
}
}