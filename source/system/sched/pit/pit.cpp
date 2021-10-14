#include <stdint.h>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/io.hpp>

bool pit_initialised = false;

uint64_t tick = 0;

void PIT_sleep(double sec)
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
    serial_info("Initialising PIT\n");

    if (pit_initialised)
    {
        serial_info("PIT has already been initialised!\n");
        return;
    }

    if (!idt_initialised)
    {
        serial_info("IDT has not been initialised!\n");
        IDT_init();
    }

    uint64_t divisor = 1193180 / 100;

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF );

    outb(0x40, l);
    outb(0x40, h);

    register_interrupt_handler(IRQ0, PIT_Handler);

    pit_initialised = true;
}