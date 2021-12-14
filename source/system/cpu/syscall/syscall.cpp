// Copyright (C) 2021  ilobilo

#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/liballoc.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <linux/reboot.h>

using namespace kernel::drivers::display;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;

namespace kernel::system::cpu::syscall {

bool initialised = false;

static void syscall_read(registers_t *regs)
{
    char *str = (char*)"Read currently not working!\n";
    S_RAX = (uint64_t)str;
}

static void syscall_write(registers_t *regs)
{
    switch (S_RDI_ARG0)
    {
        case 0:
        {
            char *str = (char*)malloc(S_RDX_ARG2 * sizeof(char));
            memcpy(str, (void*)S_RSI_ARG1, S_RDX_ARG2);
            str[S_RDX_ARG2] = 0;
            printf("%s", str);
            S_RAX = (uint64_t)str;
            free(str);
            break;
        }
        case 1:
            break;
        case 2:
        {
            char *str = (char*)malloc(S_RDX_ARG2 * sizeof(char));
            memcpy(str, (void*)S_RSI_ARG1, S_RDX_ARG2);
            str[S_RDX_ARG2] = 0;
            serial::err("%s", str);
            S_RAX = (uint64_t)str;
            free(str);
            break;
        }
        default:
            break;
    }
}

static void syscall_getpid(registers_t *regs)
{
    S_RAX = scheduler::getpid();
}

static void syscall_exit(registers_t *regs)
{
    scheduler::exit();
    S_RAX = 0;
}

static void syscall_reboot(registers_t *regs)
{
    if (S_RDI_ARG0 != LINUX_REBOOT_MAGIC1 || (S_RSI_ARG1 != LINUX_REBOOT_MAGIC2 && S_RSI_ARG1 != LINUX_REBOOT_MAGIC2A && S_RSI_ARG1 != LINUX_REBOOT_MAGIC2B && S_RSI_ARG1 != LINUX_REBOOT_MAGIC2C)) S_RAX = -1;
    else
    {
        switch (S_RDX_ARG2)
        {
            case LINUX_REBOOT_CMD_CAD_OFF: break;
            case LINUX_REBOOT_CMD_CAD_ON:
                acpi::reboot();
                break;
            case LINUX_REBOOT_CMD_HALT:
                printf("\nSystem halted.\n");
                serial::info("System halted.");
                asm volatile ("cli; hlt");
                break;
            case LINUX_REBOOT_CMD_KEXEC: break;
            case LINUX_REBOOT_CMD_POWER_OFF:
                printf("\nPower down.\n");
                serial::info("Power down.");
                acpi::shutdown();
                break;
            case LINUX_REBOOT_CMD_RESTART:
                printf("\nRestarting system.\n");
                serial::info("Restarting system.");
                acpi::reboot();
                break;
            case LINUX_REBOOT_CMD_RESTART2:
                printf("\nRestarting system with command '%s'.\n", S_R10_ARG3);
                serial::info("Restarting system with command '%s'.", S_R10_ARG3);
                acpi::reboot();
                break;
            case LINUX_REBOOT_CMD_SW_SUSPEND: break;
        }
        S_RAX = 0;
    }
}

syscall_t syscalls[] = {
    [0] = syscall_read,
    [1] = syscall_write,
    [39] = syscall_getpid,
    [60] = syscall_exit,
    [169] = syscall_reboot
};

static void handler(registers_t *regs)
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

void init()
{
    serial::info("Initialising System calls");

    if (initialised)
    {
        serial::warn("System calls have already been initialised!\n");
        return;
    }

    idt::register_interrupt_handler(SYSCALL, handler);

    serial::newline();
    initialised = true;
}
}