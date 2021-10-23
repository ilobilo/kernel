#pragma once

#include <stdint.h>

namespace kernel::system::cpu::idt {

#define SYSCALL 0x80

enum PIC
{
    PIC1 = 0x20,
    PIC2 = 0xA0,
    PIC1_COMMAND = PIC1,
    PIC1_DATA = (PIC1+1),
    PIC2_COMMAND = PIC2,
    PIC2_DATA = (PIC2+1),
    PIC_EOI = 0x20
};

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void isr127();

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();

extern "C" void syscall();

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
	uint16_t    isr_low;
	uint16_t    kernel_cs;
	uint8_t	    ist;
	uint8_t     attributes;
	uint16_t    isr_mid;
	uint32_t    isr_high;
	uint32_t    reserved;
} __attribute__((packed));

struct idtr_t
{
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed));

struct interrupt_registers
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax, int_no, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));

using int_handler_t = void (*)(interrupt_registers *registers);

extern idt_entry_t idt[];
extern idtr_t idtr;

extern bool initialised;

void init();
void register_interrupt_handler(uint8_t n, int_handler_t handler);

extern "C" void isr_handler(interrupt_registers *regs);
extern "C" void irq_handler(interrupt_registers *regs);
}