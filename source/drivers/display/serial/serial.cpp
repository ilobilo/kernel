#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <lib/io.hpp>

int is_transmit_empty()
{
    return inb(COM1 + 5) & 0x20;
}

void serial_printc(char c, void *arg)
{
    while (is_transmit_empty() == 0);
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

void serial_init()
{
    //serial_printf("\033[H\033[0m\033[2J");
    serial_printf("\033[0m");
}
