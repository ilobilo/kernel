#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>

char *syscall_read(int length)
{
    char *string = (char*)malloc((length + 1) * sizeof(char));
    char *ret;
    asm volatile ("int $0x30" : "=a" (ret) : "0"(0), "D"(1), "S"(string), "d"(5) : "rcx", "r11", "memory");
    return string;
}
void syscall_write(char *string)
{
    char *ret;
    int length = strlen(string);
    asm volatile ("int $0x30" : "=a" (ret) : "0"(1), "D"(0), "S"(string), "d"(5) : "rcx", "r11", "memory");
}
void syscall_err(char *string)
{
    char *ret;
    int length = strlen(string);
    asm volatile ("int $0x30" : "=a" (ret) : "0"(1), "D"(2), "S"(string), "d"(5) : "rcx", "r11", "memory");
}

void syscall_handler(interrupt_registers *regs)
{
    switch (regs->rax)
    {
        case 0:
            switch (regs->rdi)
            {
                case 0:
                    break;
                case 1:
                {
                    char *str = "Read currently not working\n";
                    regs->rsi = (uint64_t)str;
                    //for (uint64_t i = 0; i < regs->rdx; i++)
                    //{
                    //    *((uint8_t*)(regs->rsi + i)) = getchar();
                    //}
                    break;
                }
                case 2:
                    break;
                default:
                    break;
            }
            break;
        case 1:
            switch (regs->rdi)
            {
                case 0:
                    printf("%s", (char*)regs->rsi);
                    break;
                case 1:
                    break;
                case 2:
                    serial_err("%s", (char*)regs->rsi);
                    break;
                default:
                    break;
            }
            break;
    }
}