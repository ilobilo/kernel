// Copyright (C) 2021  ilobilo

#pragma once

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

struct idt_desc_t
{
    uint16_t offset_1;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t zero;
};

struct idt_entry_t
{
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr_t
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct registers_t
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t core, int_no, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));

using int_handler_t = void (*)(registers_t *);

extern idt_entry_t idt[];
extern idtr_t idtr;

extern bool initialised;

void reload();

void init();
void register_interrupt_handler(uint8_t n, int_handler_t handler);

extern "C" void int_handler(registers_t *regs);
}