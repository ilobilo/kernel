// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/pic/pic.hpp>
#include <system/trace/trace.hpp>
#include <lib/panic.hpp>
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

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t typeattr, uint8_t ist)
{
    idt[vector].Offset1 = reinterpret_cast<uint64_t>(isr);
    idt[vector].Selector = GDT_CODE_64;
    idt[vector].IST = ist;
    idt[vector].TypeAttr = typeattr;
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

void init()
{
    log("Initialising IDT");

    if (initialised)
    {
        warn("IDT has already been initialised!\n");
        return;
    }

    idt_lock.lock();

    trace::init();

    idtr.Limit = sizeof(IDTEntry) * 256 - 1;
    idtr.Base = reinterpret_cast<uintptr_t>(&idt[0]);

    for (size_t i = 0; i < 256; i++) idt_set_descriptor(i, int_table[i]);
    idt_set_descriptor(SYSCALL, int_table[SYSCALL], 0xEE);

    pic::init();

    reload();

    serial::newline();
    initialised = true;
    idt_lock.unlock();
}

static uint8_t next_free = 48;
uint8_t alloc_vector()
{
    return (++next_free == SYSCALL ? ++next_free : next_free);
}

void register_interrupt_handler(uint8_t vector, int_handler_t handler, bool ioapic)
{
    interrupt_handlers[vector] = handler;
    if (ioapic && apic::initialised && vector > 31 && vector < 48) apic::ioapic_redirect_irq(vector - 32, vector);
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
static void exception_handler(registers_t *regs)
{
    error("System exception!");
    error("Exception: %s on CPU %zu", exception_messages[regs->int_no], (smp::initialised ? this_cpu->id : 0));
    error("Address: 0x%lX", regs->rip);
    error("Error code: 0x%lX, 0b%b", regs->error_code, regs->error_code);

    switch (regs->int_no)
    {
        // case 14:
        // {
        //     uint64_t addr = 0;
        //     asm volatile ("mov %%cr2, %0" : "=r"(addr));
        //     error("CR2: 0x%lX", addr);
        //     printf("[\033[31mPANIC\033[0m] CR2: 0x%lX\n", addr);
        // }
    }

    if (!halt)
    {
        trace::trace(false);
        serial::newline();
        return;
    }

    printf("\n[\033[31mPANIC\033[0m] System Exception!\n");
    printf("[\033[31mPANIC\033[0m] Exception: %s on CPU %zu\n", exception_messages[regs->int_no], (smp::initialised ? this_cpu->id : 0));
    printf("[\033[31mPANIC\033[0m] Address: 0x%lX\n", regs->rip);

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
    trace::trace(true);
    // if (scheduler::this_proc() && scheduler::this_thread() && scheduler::this_thread()->state == scheduler::RUNNING)
    // {
    //     asm volatile ("cli");
    //     this_cpu->current_thread->state = scheduler::READY;
    //     asm volatile ("sti");
    // }
    scheduler::kill();
    asm volatile ("cli; hlt");
}

static void irq_handler(registers_t *regs)
{
    if (apic::initialised) apic::eoi();
    else pic::eoi(regs->int_no);
    if (interrupt_handlers[regs->int_no]) interrupt_handlers[regs->int_no](regs);
}

extern "C" void int_handler(registers_t *regs)
{
    if (regs->int_no < 32) exception_handler(regs);
    else if (regs->int_no >= 32 && regs->int_no < 256) irq_handler(regs);
    else PANIC("Unknown interrupt!");
}
}