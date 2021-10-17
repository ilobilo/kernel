#include <drivers/display/terminal/terminal.hpp>
#include <stdint.h>
#include <stdbool.h>

void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %w0, %w1" : : "a" (val), "Nd" (port));
}

void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %w1" : : "a" (val), "Nd" (port));
}

uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

uint16_t inw(uint16_t port)
{
    uint16_t data;
    asm volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

uint32_t inl(uint16_t port)
{
    uint32_t data;
    asm volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

void io_wait(void)
{
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

bool are_interrupts_enabled(bool should_print)
{
    unsigned long flags;
    asm volatile ("pushf\n\t" "pop %0" : "=g"(flags));
    if (should_print)
    {
        if (flags & (1 << 9)) printf("Interrupts are enabled!\n");
        else printf("Interrupts are not enabled!\n");
    }
    return flags & (1 << 9);
}

