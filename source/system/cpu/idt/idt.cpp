// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/trace/trace.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/pic/pic.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::cpu::idt {

DEFINE_LOCK(idt_lock)
bool initialised = false;

IDTEntry idt[256];
IDTPtr idtr;

int_handler_t interrupt_handlers[256];

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t type_attr)
{
    idt[vector].Offset1 = reinterpret_cast<uint64_t>(isr);
    idt[vector].Selector = 0x28;
    idt[vector].IST = 0;
    idt[vector].TypeAttr = type_attr;
    idt[vector].Offset2 = reinterpret_cast<uint64_t>(isr) >> 16;
    idt[vector].Offset3 = reinterpret_cast<uint64_t>(isr) >> 32;
    idt[vector].Zero = 0;
}

void reload()
{
    asm volatile ("cli");
    asm volatile ("lidt %0" : : "memory"(idtr));
    asm volatile ("sti");
}

extern "C" void *int_table[];
void init()
{
    log("Initialising IDT");

    if (initialised)
    {
        warn("IDT has already been initialised!\n");
        return;
    }

    acquire_lock(idt_lock);

    trace::init();

    idtr.Limit = sizeof(IDTEntry) * 256 - 1;
    idtr.Base = reinterpret_cast<uintptr_t>(&idt[0]);

    for (size_t i = 0; i < 256; i++) idt_set_descriptor(i, int_table[i], 0x8E);

    pic::init();

    reload();

    serial::newline();
    initialised = true;
    release_lock(idt_lock);
}

void register_interrupt_handler(uint8_t vector, int_handler_t handler)
{
    interrupt_handlers[vector] = handler;
    if (apic::initialised && vector > 31 && vector < 48) apic::ioapic_redirect_irq(vector - 32, vector);
}

static const char *exception_messages[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Detected overflow",
    "Out-of-bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

static volatile bool halt = true;
void exception_handler(registers_t *regs)
{
    error("System exception! %s", (char*)exception_messages[regs->int_no & 0xff]);
    error("Error code: 0x%lX", regs->error_code);

    switch (regs->int_no)
    {
    }

    if (!halt)
    {
        trace::trace();
        serial::newline();
        return;
    }

    printf("\n[\033[31mPANIC\033[0m] System Exception!\n");
    printf("[\033[31mPANIC\033[0m] Exception: %s\n", (char*)exception_messages[regs->int_no & 0xff]);

    switch (regs->int_no)
    {
        case 8:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            printf("[\033[31mPANIC\033[0m] Error code: 0x%lX\n", regs->error_code);
            break;
    }

    printf("[\033[31mPANIC\033[0m] System halted!\n");
    error("System halted!\n");
    trace::trace();
    asm volatile ("cli; hlt");
}

void irq_handler(registers_t *regs)
{
    if (interrupt_handlers[regs->int_no]) interrupt_handlers[regs->int_no](regs);
    if (apic::initialised) apic::eoi();
    else pic::eoi(regs->int_no);
}

void int_handler(registers_t *regs)
{
    if (regs->int_no < 32) exception_handler(regs);
    irq_handler(regs);
}
}