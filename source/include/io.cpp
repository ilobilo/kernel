#include <stdint.h>
#include <stdbool.h>
#include <drivers/terminal.hpp>

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void io_wait(void)
{
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

bool are_interrupts_enabled(bool should_print)
{
    unsigned long flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(flags) );
    if (should_print)
    {
        if (flags & (1 << 9))
        {
            printf("Interrupts are enabled!");
        }
        else
        {
            printf("Interrupts are not enabled!");
        }
    }
    return flags & (1 << 9);
}

