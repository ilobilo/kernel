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

struct [[gnu::packed]] idt_entry_t
{
    uint16_t offset_1;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t zero;
};

struct [[gnu::packed]] idtr_t
{
    uint16_t limit;
    uint64_t base;
};

using int_handler_t = void (*)(registers_t *);

extern idt_entry_t idt[];
extern idtr_t idtr;

extern int_handler_t interrupt_handlers[];

extern bool initialised;

void reload();

void init();
void register_interrupt_handler(uint8_t vector, int_handler_t handler);

extern "C" void int_handler(registers_t *regs);
}