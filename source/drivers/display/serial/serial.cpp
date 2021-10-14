#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/io.hpp>

bool serial_initialised = false;

bool check_serial()
{
    if (serial_initialised) return true;
    else return false;
}

int is_transmit_empty()
{
    return inb(COM1 + 5) & 0x20;
}

int serial_received()
{
    return inb(COM1 + 5) & 1;
}

char serial_read()
{
    while (!serial_received());
    return inb(COM1);
}

void serial_printc(char c, __attribute__((unused)) void *arg)
{
    if (!check_serial()) return;
    while (!is_transmit_empty());
    outb(COM1, c);
}

void serial_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&serial_printc, nullptr, fmt, args);
    va_end(args);
}

void serial_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&serial_printc, nullptr, "[\033[33mINFO\033[0m] ", args);
    vfctprintf(&serial_printc, nullptr, fmt, args);
    vfctprintf(&serial_printc, nullptr, "\n", args);
    va_end(args);
}

void serial_err(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&serial_printc, nullptr, "[\033[31mERROR\033[0m] ", args);
    vfctprintf(&serial_printc, nullptr, fmt, args);
    vfctprintf(&serial_printc, nullptr, "\n", args);
    va_end(args);
}

void serial_newline()
{
    serial_printf("\n");
}

static void COM1_Handler(interrupt_registers *)
{
    char c = serial_read();
    switch (c)
    {
        case 13:
            c = '\n';
            break;
        case 8:
        case 127:
            printf("\b ");
            serial_printf("\b ");
            c = '\b';
            break;
    }
    printf("%c", c);
    serial_printf("%c", c);
}

void serial_init()
{
    if (serial_initialised) return;

    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    //serial_printf("\033[H\033[0m\033[2J");
    serial_printf("\033[0m");

    register_interrupt_handler(IRQ4, COM1_Handler);
    outb(COM1 + 1, 0x01);

    serial_initialised = true;
}
