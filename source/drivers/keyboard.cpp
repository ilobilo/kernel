#include <stdint.h>
#include "../system/idt/idt.hpp"
#include "../include/io.hpp"
#include "terminal.hpp"

static void Keyboard_Handler(struct interrupt_registers *)
{
    term_print("Key Pressed! ");
    uint8_t scancode = inb(0x60);
}

void Keyboard_init()
{
    register_interrupt_handler(IRQ1, Keyboard_Handler);
}
