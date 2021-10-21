#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>

static void syscall_read(interrupt_registers *regs)
{
    char *str = "Read currently not working\n";
    S_ARG1_RSI = (uint64_t)str;
}

static void syscall_write(interrupt_registers *regs)
{
    switch (S_ARG0_RDI)
    {
        case 0:
        {
            char *str = (char*)malloc(S_ARG2_RDX * sizeof(char));
            memcpy(str, (void*)S_ARG1_RSI, S_ARG2_RDX);
            str[S_ARG2_RDX] = 0;
            printf("%s", str);
            free(str);
            break;
        }
        case 1:
            break;
        case 2:
        {
            char *str = (char*)malloc(S_ARG2_RDX * sizeof(char));
            memcpy(str, (void*)S_ARG1_RSI, S_ARG2_RDX);
            str[S_ARG2_RDX] = 0;
            serial_err("%s", str);
            free(str);
            break;
        }
        default:
            break;
    }
}

uint64_t syscall_count = 2;
syscall_t syscalls[] = {
    syscall_read,
    syscall_write,
};

void syscall_handler(interrupt_registers *regs)
{
    if (S_RAX >= ZERO && S_RAX < syscall_count) syscalls[S_RAX](regs);
}

char *s_read(char *string, int length)
{
    char *ret;
    asm volatile ("int $0x80" : "=a" (ret) : "0"(0), "D"(1), "S"(string), "d"(length) : "rcx", "r11", "memory");
    return string;
}
void s_write(char *string, int length)
{
    char *ret;
    asm volatile ("int $0x80" : "=a" (ret) : "0"(1), "D"(0), "S"(string), "d"(length) : "rcx", "r11", "memory");
}
void s_err(char *string, int length)
{
    char *ret;
    asm volatile ("int $0x80" : "=a" (ret) : "0"(1), "D"(2), "S"(string), "d"(length) : "rcx", "r11", "memory");
}