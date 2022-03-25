// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/lock.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::fs;
using namespace kernel::system::cpu;
using namespace kernel::system;

namespace kernel::drivers::display::serial {

bool initialised = false;
new_lock(serial_lock);

struct serial_res : vfs::resource_t
{
    COMS com;

    int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int ioctl(void *handle, uint64_t request, void *argp);
    bool grow(void *handle, size_t new_size);
    void unref(void *handle);
    void link(void *handle);
    void unlink(void *handle);
    void *mmap(uint64_t page, int flags);
};

static bool is_transmit_empty(COMS com = COM1)
{
    return inb(com + 5) & 0x20;
}

static bool received(COMS com = COM1)
{
    return inb(com + 5) & 1;
}

static char read(COMS com = COM1)
{
    while (!received());
    return inb(com);
}

void printc(char c, void *arg)
{
    if (!initialised) return;
    while (!is_transmit_empty());
    outb(reinterpret_cast<uintptr_t>(arg), c);
}

void print(COMS com, const char *fmt, ...)
{
    lockit(serial_lock);

    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, reinterpret_cast<void*>(com), fmt, args);
    va_end(args);
}

void newline(COMS com)
{
    print(com, "\n");
}

int64_t serial_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    lockit(this->lock);
    for (size_t i = 0; i < size; i++)
    {
        buffer[i] = serial::read(this->com);
    }
    return size;
}

int64_t serial_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    lockit(this->lock);
    print(this->com, "%.*s", size, buffer);
    return size;
}

int serial_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool serial_res::grow(void *handle, size_t new_size)
{
    return false;
}

void serial_res::unref(void *handle)
{
    this->refcount--;
}

void serial_res::link(void *handle)
{
    this->stat.nlink++;
}

void serial_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *serial_res::mmap(uint64_t page, int flags)
{
    return nullptr;
}

static void initport(COMS com)
{
    outb(com + 7, 0x55);
    if (inb(com + 7) != 0x55) return;

    outb(com + 1, 0x00);
    outb(com + 3, 0x80);
    outb(com + 0, 0x01);
    outb(com + 1, 0x00);
    outb(com + 3, 0x03);
    outb(com + 2, 0xC7);
    outb(com + 4, 0x0B);

    print(com, "\033[0m");
}

void early_init()
{
    initport(COM1);
    initport(COM2);
    initport(COM3);
    initport(COM4);
}

void init()
{
    if (initialised) return;

    serial_res *res = new serial_res;
    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 512;
    res->stat.rdev = vfs::dev_new_id();
    res->stat.mode = 0644 | vfs::ifchr;
    res->com = COM1;
    devfs::add(res, "stty1");

    res = new serial_res;
    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 512;
    res->stat.rdev = vfs::dev_new_id();
    res->stat.mode = 0644 | vfs::ifchr;
    res->com = COM2;
    devfs::add(res, "stty2");

    initialised = true;
}
}