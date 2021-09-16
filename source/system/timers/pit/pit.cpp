#include <stdint.h>
#include <drivers/display/serial/serial.hpp>
#include <system/idt/idt.hpp>
#include <include/io.hpp>

uint64_t tick = 0;

void sleep(long sec)
{
    long start = tick;
    while (tick < start + sec * 100)
    {
        asm ("hlt");
    }
}

uint64_t get_tick()
{
    return tick;
}

static void PIT_Handler(interrupt_registers *)
{
    tick++;
}

void PIT_init()
{
    serial_info("Initializing PIT");

    register_interrupt_handler(IRQ0, PIT_Handler);

    uint64_t divisor = 1193180 / 100;

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF );

    outb(0x40, l);
    outb(0x40, h);

    serial_info("Initialized PIT\n");
}
