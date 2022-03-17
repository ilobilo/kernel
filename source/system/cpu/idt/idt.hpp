// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/cpu.hpp>
#include <stdint.h>

namespace kernel::system::cpu::idt {

static constexpr uint8_t SYSCALL = 0x69;

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

using int_handler_func = void (*)(registers_t *);
using int_handler_func_arg = void (*)(registers_t *, uint64_t);

struct int_handler_t
{
    uintptr_t handler;
    uint64_t args;
    bool arg = false;
};

extern int_handler_t interrupt_handlers[];

#define SET_HANDLER_ARG(x, y, z) { \
    interrupt_handlers[x].handler = reinterpret_cast<uint64_t>(y); \
    interrupt_handlers[x].arg = true; \
    interrupt_handlers[x].args = (z); \
}

#define SET_HANDLER(x, y) { \
    interrupt_handlers[x].handler = reinterpret_cast<uint64_t>(y); \
    interrupt_handlers[x].arg = false; \
}

#define GET_HANDLER(x, y) { \
    if (interrupt_handlers[x].handler) \
    { \
        if (interrupt_handlers[x].arg) \
        { \
            reinterpret_cast<int_handler_func_arg>(interrupt_handlers[x].handler)(y, interrupt_handlers[x].args); \
        } \
        else reinterpret_cast<int_handler_func>(interrupt_handlers[x].handler)(y); \
    } \
}

extern IDTEntry idt[];
extern IDTPtr idtr;

extern bool initialised;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t typeattr = 0x8E, uint8_t ist = 0);
void reload();

extern "C" void *int_table[];
void init();

uint8_t alloc_vector();

void register_interrupt_handler(uint8_t vector, int_handler_func handler, bool ioapic = true);
void register_interrupt_handler(uint8_t vector, int_handler_func_arg handler, uint64_t args, bool ioapic = true);
}