// Copyright (C) 2021  ilobilo

#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;

namespace kernel::system::cpu::syscall {

static void syscall_read(idt::interrupt_registers *regs)
{
    char *str = (char*)"Read currently not working!\n";
    S_RAX = (uint64_t)str;
}

static void syscall_write(idt::interrupt_registers *regs)
{
    switch (S_RDI_ARG0)
    {
        case 0:
        {
            char *str = (char*)heap::malloc(S_RDX_ARG2 * sizeof(char));
            memcpy(str, (void*)S_RSI_ARG1, S_RDX_ARG2);
            str[S_RDX_ARG2] = 0;
            printf("%s", str);
            S_RAX = (uint64_t)str;
            heap::free(str);
            break;
        }
        case 1:
            break;
        case 2:
        {
            char *str = (char*)heap::malloc(S_RDX_ARG2 * sizeof(char));
            memcpy(str, (void*)S_RSI_ARG1, S_RDX_ARG2);
            str[S_RDX_ARG2] = 0;
            serial::err("%s", str);
            S_RAX = (uint64_t)str;
            heap::free(str);
            break;
        }
        default:
            break;
    }
}

syscall_t syscalls[] = {
    [0] = syscall_read,
    [1] = syscall_write
};

void handler(idt::interrupt_registers *regs)
{
    if (S_RAX >= ZERO && syscalls[S_RAX]) syscalls[S_RAX](regs);
}

const char *read(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_READ, 1, (uint64_t)string, (uint64_t)length);
    return (const char*)ret;
}
const char *write(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_WRITE, 0, (uint64_t)string, (uint64_t)length);
    return (const char*)ret;
}
const char *err(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_WRITE, 2, (uint64_t)string, (uint64_t)length);
    return (const char*)ret;
}
}