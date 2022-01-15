// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/cpu.hpp>
#include <stdint.h>

namespace kernel::system::cpu::idt {

#define SYSCALL 0x80

enum IRQS
{
    IRQ0 = 32,
    IRQ1 = 33,
    IRQ2 = 34,
    IRQ3 = 35,
    IRQ4 = 36,
    IRQ5 = 37,
    IRQ6 = 38,
    IRQ7 = 39,
    IRQ8 = 40,
    IRQ9 = 41,
    IRQ10 = 42,
    IRQ11 = 43,
    IRQ12 = 44,
    IRQ13 = 45,
    IRQ14 = 46,
    IRQ15 = 47,
};

struct [[gnu::packed]] IDTEntry
{
    uint16_t Offset1;
    uint16_t Selector;
    uint8_t IST;
    uint8_t TypeAttr;
    uint16_t Offset2;
    uint32_t Offset3;
    uint32_t Zero;
};

struct [[gnu::packed]] IDTPtr
{
    uint16_t Limit;
    uint64_t Base;
};

using int_handler_t = void (*)(registers_t *);

extern IDTEntry idt[];
extern IDTPtr idtr;

extern int_handler_t interrupt_handlers[];

extern bool initialised;

void reload();

void init();

uint8_t alloc_vector();
void register_interrupt_handler(uint8_t vector, int_handler_t handler, bool ioapic = true);
}