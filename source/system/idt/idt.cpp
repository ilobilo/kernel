#include <drivers/serial/serial.hpp>
#include <drivers/terminal/terminal.hpp>
#include <include/io.hpp>
#include <system/idt/idt.hpp>

int_handler_t interrupt_handlers[256];

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t type_attr)
{
    idt_desc_t* descriptor = (idt_desc_t *)&idt[vector];

    descriptor->offset_1       = (uint64_t)isr & 0xFFFF;
    descriptor->selector       = 0x28;
    descriptor->ist            = 0;
    descriptor->type_attr      = type_attr;
    descriptor->offset_2       = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->offset_3       = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->zero           = 0;
}

void register_interrupt_handler(uint8_t n, int_handler_t handler);
static void not_implemented(struct interrupt_registers *);
void isr_install();

void IDT_init()
{
    serial_info("Initializing IDT");

    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_desc_t) * 256 - 1;

    for (int i = 32; i < 48; i++)
    {
        register_interrupt_handler(i, not_implemented);
    }

    isr_install();

    __asm__ volatile ("lidt %0" : : "memory"(idtr));
    __asm__ volatile ("sti");

    serial_info("Initialized IDT");
    serial_printc('\n');
}

void register_interrupt_handler(uint8_t n, int_handler_t handler)
{
    interrupt_handlers[n] = handler;
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

void isr_handler(struct interrupt_registers *regs)
{
    term_clear();
    term_center("[PANIC] System Exception!");
    term_center((char*)exception_messages[regs->int_no & 0xff]);

    serial_err("System exception!");
    serial_err((char*)exception_messages[regs->int_no & 0xff]);

    __asm__ volatile ("cli; hlt");
}

void irq_handler(struct interrupt_registers *regs)
{
    if (regs->int_no != 0)
    {
        int_handler_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }

    if(regs->int_no >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

static void not_implemented(struct interrupt_registers *)
{
}

void isr_install()
{
    idt_set_descriptor(0, (void*)isr0, 0x8E);
    idt_set_descriptor(1, (void*)isr1, 0x8E);
    idt_set_descriptor(2, (void*)isr2, 0x8E);
    idt_set_descriptor(3, (void*)isr3, 0x8E);
    idt_set_descriptor(4, (void*)isr4, 0x8E);
    idt_set_descriptor(5, (void*)isr5, 0x8E);
    idt_set_descriptor(6, (void*)isr6, 0x8E);
    idt_set_descriptor(7, (void*)isr7, 0x8E);
    idt_set_descriptor(8, (void*)isr8, 0x8E);
    idt_set_descriptor(9, (void*)isr9, 0x8E);
    idt_set_descriptor(10, (void*)isr10, 0x8E);
    idt_set_descriptor(11, (void*)isr11, 0x8E);
    idt_set_descriptor(12, (void*)isr12, 0x8E);
    idt_set_descriptor(13, (void*)isr13, 0x8E);
    idt_set_descriptor(14, (void*)isr14, 0x8E);
    idt_set_descriptor(15, (void*)isr15, 0x8E);
    idt_set_descriptor(16, (void*)isr16, 0x8E);
    idt_set_descriptor(17, (void*)isr17, 0x8E);
    idt_set_descriptor(18, (void*)isr18, 0x8E);
    idt_set_descriptor(19, (void*)isr19, 0x8E);
    idt_set_descriptor(20, (void*)isr20, 0x8E);
    idt_set_descriptor(21, (void*)isr21, 0x8E);
    idt_set_descriptor(22, (void*)isr22, 0x8E);
    idt_set_descriptor(23, (void*)isr23, 0x8E);
    idt_set_descriptor(24, (void*)isr24, 0x8E);
    idt_set_descriptor(25, (void*)isr25, 0x8E);
    idt_set_descriptor(26, (void*)isr26, 0x8E);
    idt_set_descriptor(27, (void*)isr27, 0x8E);
    idt_set_descriptor(28, (void*)isr28, 0x8E);
    idt_set_descriptor(29, (void*)isr29, 0x8E);
    idt_set_descriptor(30, (void*)isr30, 0x8E);
    idt_set_descriptor(31, (void*)isr31, 0x8E);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    idt_set_descriptor(32, (void*)irq0, 0x8E);
    idt_set_descriptor(33, (void*)irq1, 0x8E);
    idt_set_descriptor(34, (void*)irq2, 0x8E);
    idt_set_descriptor(35, (void*)irq3, 0x8E);
    idt_set_descriptor(36, (void*)irq4, 0x8E);
    idt_set_descriptor(37, (void*)irq5, 0x8E);
    idt_set_descriptor(38, (void*)irq6, 0x8E);
    idt_set_descriptor(39, (void*)irq7, 0x8E);
    idt_set_descriptor(40, (void*)irq8, 0x8E);
    idt_set_descriptor(41, (void*)irq9, 0x8E);
    idt_set_descriptor(42, (void*)irq10, 0x8E);
    idt_set_descriptor(43, (void*)irq11, 0x8E);
    idt_set_descriptor(44, (void*)irq12, 0x8E);
    idt_set_descriptor(45, (void*)irq13, 0x8E);
    idt_set_descriptor(46, (void*)irq14, 0x8E);
    idt_set_descriptor(47, (void*)irq15, 0x8E);
}
